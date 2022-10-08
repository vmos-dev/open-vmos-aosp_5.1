/*--------------------------------------------------------------------------
Copyright (c) 2010-2016, Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
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

*//** @file omx_video_base.cpp
  This module contains the implementation of the OpenMAX core & component.

*//*========================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "omx_video_base.h"
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/prctl.h>
#ifdef _ANDROID_ICS_
#include <media/hardware/HardwareAPI.h>
#include <gralloc_priv.h>
#endif
#ifndef _ANDROID_
#include <glib.h>
#define strlcpy g_strlcpy
#endif
#define H264_SUPPORTED_WIDTH (480)
#define H264_SUPPORTED_HEIGHT (368)

#define MPEG4_SUPPORTED_WIDTH (480)
#define MPEG4_SUPPORTED_HEIGHT (368)

#define VC1_SP_MP_START_CODE        0xC5000000
#define VC1_SP_MP_START_CODE_MASK   0xFF000000
#define VC1_AP_START_CODE           0x00000100
#define VC1_AP_START_CODE_MASK      0xFFFFFF00
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

#define SZ_4K                       0x1000
#define SZ_1M                       0x100000
#define SECURE_BUFPTR               0xDEADBEEF

typedef struct OMXComponentCapabilityFlagsType {
    ////////////////// OMX COMPONENT CAPABILITY RELATED MEMBERS
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL iIsOMXComponentMultiThreaded;
    OMX_BOOL iOMXComponentSupportsExternalOutputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsExternalInputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsMovableInputBuffers;
    OMX_BOOL iOMXComponentSupportsPartialFrames;
    OMX_BOOL iOMXComponentUsesNALStartCodes;
    OMX_BOOL iOMXComponentCanHandleIncompleteFrames;
    OMX_BOOL iOMXComponentUsesFullAVCFrames;

} OMXComponentCapabilityFlagsType;
#define OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347

void* message_thread(void *input)
{
    omx_video* omx = reinterpret_cast<omx_video*>(input);
    unsigned char id;
    int n;

    DEBUG_PRINT_LOW("omx_venc: message thread start");
    prctl(PR_SET_NAME, (unsigned long)"VideoEncMsgThread", 0, 0, 0);
    while (1) {
        n = read(omx->m_pipe_in, &id, 1);
        if (0 == n) {
            break;
        }

        if (1 == n) {
            omx->process_event_cb(omx, id);
        }
#ifdef QLE_BUILD
        if (n < 0) break;
#else
        if ((n < 0) && (errno != EINTR)) break;
#endif
    }
    DEBUG_PRINT_LOW("omx_venc: message thread stop");
    return 0;
}

void post_message(omx_video *omx, unsigned char id)
{
    DEBUG_PRINT_LOW("omx_venc: post_message %d", id);
    write(omx->m_pipe_out, &id, 1);
}

// omx_cmd_queue destructor
omx_video::omx_cmd_queue::~omx_cmd_queue()
{
    // Nothing to do
}

// omx cmd queue constructor
omx_video::omx_cmd_queue::omx_cmd_queue(): m_read(0),m_write(0),m_size(0)
{
    memset(m_q,0,sizeof(omx_event)*OMX_CORE_CONTROL_CMDQ_SIZE);
}

// omx cmd queue insert
bool omx_video::omx_cmd_queue::insert_entry(unsigned long p1, unsigned long p2, unsigned long id)
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
        DEBUG_PRINT_ERROR("ERROR!!! Command Queue Full");
    }
    return ret;
}

// omx cmd queue pop
bool omx_video::omx_cmd_queue::pop_entry(unsigned long *p1, unsigned long *p2, unsigned long *id)
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
unsigned omx_video::omx_cmd_queue::get_q_msg_type()
{
    return m_q[m_read].id;
}



#ifdef _ANDROID_
VideoHeap::VideoHeap(int fd, size_t size, void* base)
{
    // dup file descriptor, map once, use pmem
    init(dup(fd), base, size, 0 , MEM_DEVICE);
}
#endif // _ANDROID_

/* ======================================================================
   FUNCTION
   omx_venc::omx_venc

   DESCRIPTION
   Constructor

   PARAMETERS
   None

   RETURN VALUE
   None.
   ========================================================================== */
omx_video::omx_video():
    c2d_opened(false),
    mUsesColorConversion(false),
    psource_frame(NULL),
    pdest_frame(NULL),
    secure_session(false),
    mEmptyEosBuffer(NULL),
    m_pInput_pmem(NULL),
    m_pOutput_pmem(NULL),
#ifdef USE_ION
    m_pInput_ion(NULL),
    m_pOutput_ion(NULL),
#endif
    m_error_propogated(false),
    m_state(OMX_StateInvalid),
    m_app_data(NULL),
    m_use_input_pmem(OMX_FALSE),
    m_use_output_pmem(OMX_FALSE),
    m_input_msg_id(OMX_COMPONENT_GENERATE_ETB),
    m_inp_mem_ptr(NULL),
    m_out_mem_ptr(NULL),
    input_flush_progress (false),
    output_flush_progress (false),
    input_use_buffer (false),
    output_use_buffer (false),
    pending_input_buffers(0),
    pending_output_buffers(0),
    m_out_bm_count(0),
    m_inp_bm_count(0),
    m_flags(0),
    m_etb_count(0),
    m_fbd_count(0),
    m_event_port_settings_sent(false)
{
    DEBUG_PRINT_HIGH("omx_video(): Inside Constructor()");
    memset(&m_cmp,0,sizeof(m_cmp));
    memset(&m_pCallbacks,0,sizeof(m_pCallbacks));
    async_thread_created = false;
    msg_thread_created = false;

    pthread_mutex_init(&m_lock, NULL);
    sem_init(&m_cmd_lock,0,0);
}


/* ======================================================================
   FUNCTION
   omx_venc::~omx_venc

   DESCRIPTION
   Destructor

   PARAMETERS
   None

   RETURN VALUE
   None.
   ========================================================================== */
omx_video::~omx_video()
{
    DEBUG_PRINT_HIGH("~omx_video(): Inside Destructor()");
    if (m_pipe_in) close(m_pipe_in);
    if (m_pipe_out) close(m_pipe_out);
    DEBUG_PRINT_HIGH("omx_video: Waiting on Msg Thread exit");
    if (msg_thread_created)
        pthread_join(msg_thread_id,NULL);
    DEBUG_PRINT_HIGH("omx_video: Waiting on Async Thread exit");
    /*For V4L2 based drivers, pthread_join is done in device_close
     * so no need to do it here*/
#ifndef _MSM8974_
    if (async_thread_created)
        pthread_join(async_thread_id,NULL);
#endif
    pthread_mutex_destroy(&m_lock);
    sem_destroy(&m_cmd_lock);
    DEBUG_PRINT_HIGH("m_etb_count = %u, m_fbd_count = %u", m_etb_count,
            m_fbd_count);
    DEBUG_PRINT_HIGH("omx_video: Destructor exit");
    DEBUG_PRINT_HIGH("Exiting 7x30 OMX Video Encoder ...");
}

/* ======================================================================
   FUNCTION
   omx_venc::OMXCntrlProcessMsgCb

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
void omx_video::process_event_cb(void *ctxt, unsigned char id)
{
    unsigned long p1; // Parameter - 1
    unsigned long p2; // Parameter - 2
    unsigned long ident;
    unsigned qsize=0; // qsize
    omx_video *pThis = (omx_video *) ctxt;

    if (!pThis) {
        DEBUG_PRINT_ERROR("ERROR:ProcessMsgCb:Context is incorrect; bailing out");
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

        if (qsize == 0) {
            qsize = pThis->m_ftb_q.m_size;
            if (qsize) {
                pThis->m_ftb_q.pop_entry(&p1,&p2,&ident);
            }
        }

        if (qsize == 0) {
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
                    if (pThis->m_pCallbacks.EventHandler) {
                        switch (p1) {
                            case OMX_CommandStateSet:
                                pThis->m_state = (OMX_STATETYPE) p2;
                                DEBUG_PRINT_LOW("Process -> state set to %d", pThis->m_state);
                                pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                        OMX_EventCmdComplete, p1, p2, NULL);
                                break;

                            case OMX_EventError:
                                DEBUG_PRINT_ERROR("ERROR: OMX_EventError: p2 = %lu", p2);
                                if (p2 == (unsigned)OMX_ErrorHardware) {
                                    pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                            OMX_EventError,OMX_ErrorHardware,0,NULL);
                                } else {
                                    pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                            OMX_EventError, p2, 0, 0);

                                }
                                break;

                            case OMX_CommandPortDisable:
                                DEBUG_PRINT_LOW("Process -> Port %lu set to PORT_STATE_DISABLED" \
                                        "state", p2);
                                pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                        OMX_EventCmdComplete, p1, p2, NULL );
                                break;
                            case OMX_CommandPortEnable:
                                DEBUG_PRINT_LOW("Process ->Port %lu set PORT_STATE_ENABLED state" \
                                        , p2);
                                pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,\
                                        OMX_EventCmdComplete, p1, p2, NULL );
                                break;

                            default:
                                DEBUG_PRINT_LOW("process_event_cb forwarding EventCmdComplete %lu", p1);
                                pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                        OMX_EventCmdComplete, p1, p2, NULL );
                                break;

                        }
                    } else {
                        DEBUG_PRINT_ERROR("ERROR: ProcessMsgCb NULL callbacks");
                    }
                    break;
                case OMX_COMPONENT_GENERATE_ETB_OPQ:
                    DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_ETB_OPQ");
                    if (pThis->empty_this_buffer_opaque((OMX_HANDLETYPE)p1,\
                                (OMX_BUFFERHEADERTYPE *)p2) != OMX_ErrorNone) {
                        DEBUG_PRINT_ERROR("ERROR: ETBProxy() failed!");
                        pThis->omx_report_error ();
                    }
                    break;
                case OMX_COMPONENT_GENERATE_ETB:
                    DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_ETB");
                    if (pThis->empty_this_buffer_proxy((OMX_HANDLETYPE)p1,\
                                (OMX_BUFFERHEADERTYPE *)p2) != OMX_ErrorNone) {
                        DEBUG_PRINT_ERROR("ERROR: ETBProxy() failed!");
                        pThis->omx_report_error ();
                    }
                    break;

                case OMX_COMPONENT_GENERATE_FTB:
                    if ( pThis->fill_this_buffer_proxy((OMX_HANDLETYPE)p1,\
                                (OMX_BUFFERHEADERTYPE *)p2) != OMX_ErrorNone) {
                        DEBUG_PRINT_ERROR("ERROR: FTBProxy() failed!");
                        pThis->omx_report_error ();
                    }
                    break;

                case OMX_COMPONENT_GENERATE_COMMAND:
                    pThis->send_command_proxy(&pThis->m_cmp,(OMX_COMMANDTYPE)p1,\
                            (OMX_U32)p2,(OMX_PTR)NULL);
                    break;

                case OMX_COMPONENT_GENERATE_EBD:
                    if ( pThis->empty_buffer_done(&pThis->m_cmp,
                                (OMX_BUFFERHEADERTYPE *)p1) != OMX_ErrorNone) {
                        DEBUG_PRINT_ERROR("ERROR: empty_buffer_done() failed!");
                        pThis->omx_report_error ();
                    }
                    break;

                case OMX_COMPONENT_GENERATE_FBD:
                    if ( pThis->fill_buffer_done(&pThis->m_cmp,
                                (OMX_BUFFERHEADERTYPE *)p1) != OMX_ErrorNone ) {
                        DEBUG_PRINT_ERROR("ERROR: fill_buffer_done() failed!");
                        pThis->omx_report_error ();
                    }
                    break;

                case OMX_COMPONENT_GENERATE_EVENT_INPUT_FLUSH:

                    pThis->input_flush_progress = false;
                    DEBUG_PRINT_HIGH("m_etb_count at i/p flush = %u", m_etb_count);
                    m_etb_count = 0;
                    if (pThis->m_pCallbacks.EventHandler) {
                        /*Check if we need generate event for Flush done*/
                        if (BITMASK_PRESENT(&pThis->m_flags,
                                    OMX_COMPONENT_INPUT_FLUSH_PENDING)) {
                            BITMASK_CLEAR (&pThis->m_flags,OMX_COMPONENT_INPUT_FLUSH_PENDING);
                            pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                    OMX_EventCmdComplete,OMX_CommandFlush,
                                    PORT_INDEX_IN,NULL );
                        } else if (BITMASK_PRESENT(&pThis->m_flags,
                                    OMX_COMPONENT_IDLE_PENDING)) {
                            if (!pThis->output_flush_progress) {
                                DEBUG_PRINT_LOW("dev_stop called after input flush complete");
                                if (dev_stop() != 0) {
                                    DEBUG_PRINT_ERROR("ERROR: dev_stop() failed in i/p flush!");
                                    pThis->omx_report_error ();
                                }
                            }
                        }
                    }

                    break;

                case OMX_COMPONENT_GENERATE_EVENT_OUTPUT_FLUSH:

                    pThis->output_flush_progress = false;
                    DEBUG_PRINT_HIGH("m_fbd_count at o/p flush = %u", m_fbd_count);
                    m_fbd_count = 0;
                    if (pThis->m_pCallbacks.EventHandler) {
                        /*Check if we need generate event for Flush done*/
                        if (BITMASK_PRESENT(&pThis->m_flags,
                                    OMX_COMPONENT_OUTPUT_FLUSH_PENDING)) {
                            BITMASK_CLEAR (&pThis->m_flags,OMX_COMPONENT_OUTPUT_FLUSH_PENDING);

                            pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                    OMX_EventCmdComplete,OMX_CommandFlush,
                                    PORT_INDEX_OUT,NULL );
                        } else if (BITMASK_PRESENT(&pThis->m_flags ,OMX_COMPONENT_IDLE_PENDING)) {
                            DEBUG_PRINT_LOW("dev_stop called after Output flush complete");
                            if (!pThis->input_flush_progress) {
                                if (dev_stop() != 0) {
                                    DEBUG_PRINT_ERROR("ERROR: dev_stop() failed in o/p flush!");
                                    pThis->omx_report_error ();
                                }
                            }
                        }
                    }
                    break;

                case OMX_COMPONENT_GENERATE_START_DONE:
                    DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_START_DONE msg");

                    if (pThis->m_pCallbacks.EventHandler) {
                        DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_START_DONE Success");
                        if (BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_EXECUTE_PENDING)) {
                            DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_START_DONE Move to \
                                    executing");
                            // Send the callback now
                            BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_EXECUTE_PENDING);
                            pThis->m_state = OMX_StateExecuting;
                            pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                    OMX_EventCmdComplete,OMX_CommandStateSet,
                                    OMX_StateExecuting, NULL);
                        } else if (BITMASK_PRESENT(&pThis->m_flags,
                                    OMX_COMPONENT_PAUSE_PENDING)) {
                            if (dev_pause()) {
                                DEBUG_PRINT_ERROR("ERROR: dev_pause() failed in Start Done!");
                                pThis->omx_report_error ();
                            }
                        } else if (BITMASK_PRESENT(&pThis->m_flags,
                                    OMX_COMPONENT_LOADED_START_PENDING)) {
                            if (dev_loaded_start_done()) {
                                DEBUG_PRINT_LOW("successful loaded Start Done!");
                            } else {
                                DEBUG_PRINT_ERROR("ERROR: failed in loaded Start Done!");
                                pThis->omx_report_error ();
                            }
                            BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_LOADED_START_PENDING);
                        } else {
                            DEBUG_PRINT_LOW("ERROR: unknown flags=%x",pThis->m_flags);
                        }
                    } else {
                        DEBUG_PRINT_LOW("Event Handler callback is NULL");
                    }
                    break;

                case OMX_COMPONENT_GENERATE_PAUSE_DONE:
                    DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_PAUSE_DONE msg");
                    if (pThis->m_pCallbacks.EventHandler) {
                        if (BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_PAUSE_PENDING)) {
                            //Send the callback now
                            pThis->complete_pending_buffer_done_cbs();
                            DEBUG_PRINT_LOW("omx_video::process_event_cb() Sending PAUSE complete after all pending EBD/FBD");
                            BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_PAUSE_PENDING);
                            pThis->m_state = OMX_StatePause;
                            pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                    OMX_EventCmdComplete,OMX_CommandStateSet,
                                    OMX_StatePause, NULL);
                        }
                    }

                    break;

                case OMX_COMPONENT_GENERATE_RESUME_DONE:
                    DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_RESUME_DONE msg");
                    if (pThis->m_pCallbacks.EventHandler) {
                        if (BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_EXECUTE_PENDING)) {
                            // Send the callback now
                            BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_EXECUTE_PENDING);
                            pThis->m_state = OMX_StateExecuting;
                            pThis->m_pCallbacks.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                    OMX_EventCmdComplete,OMX_CommandStateSet,
                                    OMX_StateExecuting,NULL);
                        }
                    }

                    break;

                case OMX_COMPONENT_GENERATE_STOP_DONE:
                    DEBUG_PRINT_LOW("OMX_COMPONENT_GENERATE_STOP_DONE msg");
                    if (pThis->m_pCallbacks.EventHandler) {
                        pThis->complete_pending_buffer_done_cbs();
                        if (BITMASK_PRESENT(&pThis->m_flags,OMX_COMPONENT_IDLE_PENDING)) {
                            // Send the callback now
                            BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_IDLE_PENDING);
                            pThis->m_state = OMX_StateIdle;
                            pThis->m_pCallbacks.EventHandler(&pThis->m_cmp,pThis->m_app_data,
                                    OMX_EventCmdComplete,OMX_CommandStateSet,
                                    OMX_StateIdle,NULL);
                        } else if (BITMASK_PRESENT(&pThis->m_flags,
                                    OMX_COMPONENT_LOADED_STOP_PENDING)) {
                            if (dev_loaded_stop_done()) {
                                DEBUG_PRINT_LOW("successful loaded Stop Done!");
                            } else {
                                DEBUG_PRINT_ERROR("ERROR: failed in loaded Stop Done!");
                                pThis->omx_report_error ();
                            }
                            BITMASK_CLEAR((&pThis->m_flags),OMX_COMPONENT_LOADED_STOP_PENDING);
                        } else {
                            DEBUG_PRINT_LOW("ERROR: unknown flags=%x",pThis->m_flags);
                        }
                    }

                    break;

                case OMX_COMPONENT_GENERATE_HARDWARE_ERROR:
                    DEBUG_PRINT_ERROR("ERROR: OMX_COMPONENT_GENERATE_HARDWARE_ERROR!");
                    pThis->omx_report_error ();
                    break;
#ifndef _MSM8974_
                case OMX_COMPONENT_GENERATE_LTRUSE_FAILED:
                    DEBUG_PRINT_ERROR("ERROR: OMX_COMPONENT_GENERATE_LTRUSE_FAILED!");
                    if (pThis->m_pCallbacks.EventHandler) {
                        DEBUG_PRINT_ERROR("Sending QOMX_ErrorLTRUseFailed, p2 = 0x%x", p2);
                        pThis->m_pCallbacks.EventHandler(
                                &pThis->m_cmp, pThis->m_app_data,
                                OMX_EventError, QOMX_ErrorLTRUseFailed, NULL, NULL);
                    }
                    break;
#endif
                default:
                    DEBUG_PRINT_LOW("process_event_cb unknown msg id 0x%02x", id);
                    break;
            }
        }

        pthread_mutex_lock(&pThis->m_lock);
        qsize = pThis->m_cmd_q.m_size + pThis->m_ftb_q.m_size +\
                pThis->m_etb_q.m_size;

        pthread_mutex_unlock(&pThis->m_lock);

    } while (qsize>0);
    DEBUG_PRINT_LOW("exited the while loop");

}




/* ======================================================================
   FUNCTION
   omx_venc::GetComponentVersion

   DESCRIPTION
   Returns the component version.

   PARAMETERS
   TBD.

   RETURN VALUE
   OMX_ErrorNone.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::get_component_version
(
 OMX_IN OMX_HANDLETYPE hComp,
 OMX_OUT OMX_STRING componentName,
 OMX_OUT OMX_VERSIONTYPE* componentVersion,
 OMX_OUT OMX_VERSIONTYPE* specVersion,
 OMX_OUT OMX_UUIDTYPE* componentUUID
 )
{
    (void)hComp;
    (void)componentName;
    (void)componentVersion;
    (void)componentUUID;
    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("ERROR: Get Comp Version in Invalid State");
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
   omx_venc::SendCommand

   DESCRIPTION
   Returns zero if all the buffers released..

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
OMX_ERRORTYPE  omx_video::send_command(OMX_IN OMX_HANDLETYPE hComp,
        OMX_IN OMX_COMMANDTYPE cmd,
        OMX_IN OMX_U32 param1,
        OMX_IN OMX_PTR cmdData
        )
{
    (void)hComp;
    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("ERROR: Send Command in Invalid State");
        return OMX_ErrorInvalidState;
    }

    if (cmd == OMX_CommandFlush || cmd == OMX_CommandPortDisable || cmd == OMX_CommandPortEnable) {
        if ((param1 != (OMX_U32)PORT_INDEX_IN) && (param1 != (OMX_U32)PORT_INDEX_OUT) && (param1 != (OMX_U32)PORT_INDEX_BOTH)) {
            DEBUG_PRINT_ERROR("ERROR: omx_video::send_command-->bad port index");
            return OMX_ErrorBadPortIndex;
        }
    }
    if (cmd == OMX_CommandMarkBuffer) {
        if (param1 != PORT_INDEX_IN) {
            DEBUG_PRINT_ERROR("ERROR: omx_video::send_command-->bad port index");
            return OMX_ErrorBadPortIndex;
        }
        if (!cmdData) {
            DEBUG_PRINT_ERROR("ERROR: omx_video::send_command-->param is null");
            return OMX_ErrorBadParameter;
        }
    }

    post_event((unsigned long)cmd,(unsigned long)param1,OMX_COMPONENT_GENERATE_COMMAND);
    sem_wait(&m_cmd_lock);
    return OMX_ErrorNone;
}

/* ======================================================================
   FUNCTION
   omx_venc::SendCommand

   DESCRIPTION
   Returns zero if all the buffers released..

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
OMX_ERRORTYPE  omx_video::send_command_proxy(OMX_IN OMX_HANDLETYPE hComp,
        OMX_IN OMX_COMMANDTYPE cmd,
        OMX_IN OMX_U32 param1,
        OMX_IN OMX_PTR cmdData
        )
{
    (void)hComp;
    (void)cmdData;

    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    OMX_STATETYPE eState = (OMX_STATETYPE) param1;
    int bFlag = 1;

    if (cmd == OMX_CommandStateSet) {
        /***************************/
        /* Current State is Loaded */
        /***************************/
        if (m_state == OMX_StateLoaded) {
            if (eState == OMX_StateIdle) {
                //if all buffers are allocated or all ports disabled
                if (allocate_done() ||
                        ( m_sInPortDef.bEnabled == OMX_FALSE && m_sOutPortDef.bEnabled == OMX_FALSE)) {
                    DEBUG_PRINT_LOW("OMXCORE-SM: Loaded-->Idle");
                } else {
                    DEBUG_PRINT_LOW("OMXCORE-SM: Loaded-->Idle-Pending");
                    BITMASK_SET(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                    // Skip the event notification
                    bFlag = 0;
                }
            }
            /* Requesting transition from Loaded to Loaded */
            else if (eState == OMX_StateLoaded) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Loaded-->Loaded");
                post_event(OMX_EventError,OMX_ErrorSameState,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorSameState;
            }
            /* Requesting transition from Loaded to WaitForResources */
            else if (eState == OMX_StateWaitForResources) {
                /* Since error is None , we will post an event
                   at the end of this function definition */
                DEBUG_PRINT_LOW("OMXCORE-SM: Loaded-->WaitForResources");
            }
            /* Requesting transition from Loaded to Executing */
            else if (eState == OMX_StateExecuting) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Loaded-->Executing");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Loaded to Pause */
            else if (eState == OMX_StatePause) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Loaded-->Pause");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Loaded to Invalid */
            else if (eState == OMX_StateInvalid) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Loaded-->Invalid");
                post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorInvalidState;
            } else {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Loaded-->%d Not Handled",\
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
                    DEBUG_PRINT_LOW("OMXCORE-SM: Idle-->Loaded");
                    if (dev_stop() != 0) {
                        DEBUG_PRINT_ERROR("ERROR: dev_stop() failed at Idle --> Loaded");
                        eRet = OMX_ErrorHardware;
                    }
                } else {
                    DEBUG_PRINT_LOW("OMXCORE-SM: Idle-->Loaded-Pending");
                    BITMASK_SET(&m_flags, OMX_COMPONENT_LOADING_PENDING);
                    // Skip the event notification
                    bFlag = 0;
                }
            }
            /* Requesting transition from Idle to Executing */
            else if (eState == OMX_StateExecuting) {
                if ( dev_start() ) {
                    DEBUG_PRINT_ERROR("ERROR: dev_start() failed in SCP on Idle --> Exe");
                    omx_report_error ();
                    eRet = OMX_ErrorHardware;
                } else {
                    BITMASK_SET(&m_flags,OMX_COMPONENT_EXECUTE_PENDING);
                    DEBUG_PRINT_LOW("OMXCORE-SM: Idle-->Executing");
                    bFlag = 0;
                }

                dev_start_done();
            }
            /* Requesting transition from Idle to Idle */
            else if (eState == OMX_StateIdle) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Idle-->Idle");
                post_event(OMX_EventError,OMX_ErrorSameState,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorSameState;
            }
            /* Requesting transition from Idle to WaitForResources */
            else if (eState == OMX_StateWaitForResources) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Idle-->WaitForResources");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Idle to Pause */
            else if (eState == OMX_StatePause) {
                /*To pause the Video core we need to start the driver*/
                if ( dev_start() ) {
                    DEBUG_PRINT_ERROR("ERROR: dev_start() failed in SCP on Idle --> Pause");
                    omx_report_error ();
                    eRet = OMX_ErrorHardware;
                } else {
                    BITMASK_SET(&m_flags,OMX_COMPONENT_PAUSE_PENDING);
                    DEBUG_PRINT_LOW("OMXCORE-SM: Idle-->Pause");
                    bFlag = 0;
                }
            }
            /* Requesting transition from Idle to Invalid */
            else if (eState == OMX_StateInvalid) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Idle-->Invalid");
                post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorInvalidState;
            } else {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Idle --> %d Not Handled",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }

        /******************************/
        /* Current State is Executing */
        /******************************/
        else if (m_state == OMX_StateExecuting) {
            /* Requesting transition from Executing to Idle */
            if (eState == OMX_StateIdle) {
                /* Since error is None , we will post an event
                   at the end of this function definition
                 */
                DEBUG_PRINT_LOW("OMXCORE-SM: Executing --> Idle");
                //here this should be Pause-Idle pending and should be cleared when flush is complete and change the state to Idle
                BITMASK_SET(&m_flags,OMX_COMPONENT_IDLE_PENDING);
                execute_omx_flush(OMX_ALL);
                bFlag = 0;
            }
            /* Requesting transition from Executing to Paused */
            else if (eState == OMX_StatePause) {

                if (dev_pause()) {
                    DEBUG_PRINT_ERROR("ERROR: dev_pause() failed in SCP on Exe --> Pause");
                    post_event(OMX_EventError,OMX_ErrorHardware,\
                            OMX_COMPONENT_GENERATE_EVENT);
                    eRet = OMX_ErrorHardware;
                } else {
                    BITMASK_SET(&m_flags,OMX_COMPONENT_PAUSE_PENDING);
                    DEBUG_PRINT_LOW("OMXCORE-SM: Executing-->Pause");
                    bFlag = 0;
                }
            }
            /* Requesting transition from Executing to Loaded */
            else if (eState == OMX_StateLoaded) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Executing --> Loaded");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Executing to WaitForResources */
            else if (eState == OMX_StateWaitForResources) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Executing --> WaitForResources");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Executing to Executing */
            else if (eState == OMX_StateExecuting) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Executing --> Executing");
                post_event(OMX_EventError,OMX_ErrorSameState,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorSameState;
            }
            /* Requesting transition from Executing to Invalid */
            else if (eState == OMX_StateInvalid) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Executing --> Invalid");
                post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorInvalidState;
            } else {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Executing --> %d Not Handled",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /***************************/
        /* Current State is Pause  */
        /***************************/
        else if (m_state == OMX_StatePause) {
            /* Requesting transition from Pause to Executing */
            if (eState == OMX_StateExecuting) {
                DEBUG_PRINT_LOW("Pause --> Executing");
                if ( dev_resume() ) {
                    post_event(OMX_EventError,OMX_ErrorHardware,\
                            OMX_COMPONENT_GENERATE_EVENT);
                    eRet = OMX_ErrorHardware;
                } else {
                    BITMASK_SET(&m_flags,OMX_COMPONENT_EXECUTE_PENDING);
                    DEBUG_PRINT_LOW("OMXCORE-SM: Pause-->Executing");
                    post_event (0, 0, OMX_COMPONENT_GENERATE_RESUME_DONE);
                    bFlag = 0;
                }
            }
            /* Requesting transition from Pause to Idle */
            else if (eState == OMX_StateIdle) {
                /* Since error is None , we will post an event
                   at the end of this function definition */
                DEBUG_PRINT_LOW("Pause --> Idle");
                BITMASK_SET(&m_flags,OMX_COMPONENT_IDLE_PENDING);
                execute_omx_flush(OMX_ALL);
                bFlag = 0;
            }
            /* Requesting transition from Pause to loaded */
            else if (eState == OMX_StateLoaded) {
                DEBUG_PRINT_ERROR("ERROR: Pause --> loaded");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Pause to WaitForResources */
            else if (eState == OMX_StateWaitForResources) {
                DEBUG_PRINT_ERROR("ERROR: Pause --> WaitForResources");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from Pause to Pause */
            else if (eState == OMX_StatePause) {
                DEBUG_PRINT_ERROR("ERROR: Pause --> Pause");
                post_event(OMX_EventError,OMX_ErrorSameState,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorSameState;
            }
            /* Requesting transition from Pause to Invalid */
            else if (eState == OMX_StateInvalid) {
                DEBUG_PRINT_ERROR("ERROR: Pause --> Invalid");
                post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorInvalidState;
            } else {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Paused --> %d Not Handled",eState);
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
                DEBUG_PRINT_LOW("OMXCORE-SM: WaitForResources-->Loaded");
            }
            /* Requesting transition from WaitForResources to WaitForResources */
            else if (eState == OMX_StateWaitForResources) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: WaitForResources-->WaitForResources");
                post_event(OMX_EventError,OMX_ErrorSameState,
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorSameState;
            }
            /* Requesting transition from WaitForResources to Executing */
            else if (eState == OMX_StateExecuting) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: WaitForResources-->Executing");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from WaitForResources to Pause */
            else if (eState == OMX_StatePause) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: WaitForResources-->Pause");
                post_event(OMX_EventError,OMX_ErrorIncorrectStateTransition,\
                        OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            /* Requesting transition from WaitForResources to Invalid */
            else if (eState == OMX_StateInvalid) {
                DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: WaitForResources-->Invalid");
                post_event(OMX_EventError,eState,OMX_COMPONENT_GENERATE_EVENT);
                eRet = OMX_ErrorInvalidState;
            }
            /* Requesting transition from WaitForResources to Loaded -
               is NOT tested by Khronos TS */

        } else {
            DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: %d --> %d(Not Handled)",m_state,eState);
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
            DEBUG_PRINT_ERROR("ERROR: OMXCORE-SM: Invalid -->Loaded");
            post_event(OMX_EventError,OMX_ErrorInvalidState,\
                    OMX_COMPONENT_GENERATE_EVENT);
            eRet = OMX_ErrorInvalidState;
        }
    } else if (cmd == OMX_CommandFlush) {
        if (0 == param1 || OMX_ALL == param1) {
            BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_FLUSH_PENDING);
        }
        if (1 == param1 || OMX_ALL == param1) {
            //generate output flush event only.
            BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_FLUSH_PENDING);
        }

        execute_omx_flush(param1);
        bFlag = 0;
    } else if ( cmd == OMX_CommandPortEnable) {
        if (param1 == PORT_INDEX_IN || param1 == OMX_ALL) {
            m_sInPortDef.bEnabled = OMX_TRUE;

            if ( (m_state == OMX_StateLoaded &&
                        !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                    || allocate_input_done()) {
                post_event(OMX_CommandPortEnable,PORT_INDEX_IN,
                        OMX_COMPONENT_GENERATE_EVENT);
            } else {
                DEBUG_PRINT_LOW("OMXCORE-SM: Disabled-->Enabled Pending");
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING);
                // Skip the event notification
                bFlag = 0;
            }
        }
        if (param1 == PORT_INDEX_OUT || param1 == OMX_ALL) {
            m_sOutPortDef.bEnabled = OMX_TRUE;

            if ( (m_state == OMX_StateLoaded &&
                        !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                    || (allocate_output_done())) {
                post_event(OMX_CommandPortEnable,PORT_INDEX_OUT,
                        OMX_COMPONENT_GENERATE_EVENT);

            } else {
                DEBUG_PRINT_LOW("OMXCORE-SM: Disabled-->Enabled Pending");
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                // Skip the event notification
                bFlag = 0;
            }
        }
    } else if (cmd == OMX_CommandPortDisable) {
        if (param1 == PORT_INDEX_IN || param1 == OMX_ALL) {
            m_sInPortDef.bEnabled = OMX_FALSE;
            if ((m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
                    && release_input_done()) {
                post_event(OMX_CommandPortDisable,PORT_INDEX_IN,
                        OMX_COMPONENT_GENERATE_EVENT);
            } else {
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
                if (m_state == OMX_StatePause ||m_state == OMX_StateExecuting) {
                    execute_omx_flush(PORT_INDEX_IN);
                }

                // Skip the event notification
                bFlag = 0;
            }
        }
        if (param1 == PORT_INDEX_OUT || param1 == OMX_ALL) {
            m_sOutPortDef.bEnabled = OMX_FALSE;

            if ((m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
                    && release_output_done()) {
                post_event(OMX_CommandPortDisable,PORT_INDEX_OUT,\
                        OMX_COMPONENT_GENERATE_EVENT);
            } else {
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
                if (m_state == OMX_StatePause ||m_state == OMX_StateExecuting) {
                    execute_omx_flush(PORT_INDEX_OUT);
                }
                // Skip the event notification
                bFlag = 0;

            }
        }
    } else {
        DEBUG_PRINT_ERROR("ERROR: Invalid Command received other than StateSet (%d)",cmd);
        eRet = OMX_ErrorNotImplemented;
    }
    if (eRet == OMX_ErrorNone && bFlag) {
        post_event(cmd,eState,OMX_COMPONENT_GENERATE_EVENT);
    }
    sem_post(&m_cmd_lock);
    return eRet;
}

/* ======================================================================
   FUNCTION
   omx_venc::ExecuteOmxFlush

   DESCRIPTION
   Executes the OMX flush.

   PARAMETERS
   flushtype - input flush(1)/output flush(0)/ both.

   RETURN VALUE
   true/false

   ========================================================================== */
bool omx_video::execute_omx_flush(OMX_U32 flushType)
{
    bool bRet = false;
    DEBUG_PRINT_LOW("execute_omx_flush -  %u", (unsigned int)flushType);
#ifdef _MSM8974_
    /* XXX: The driver/hardware does not support flushing of individual ports
     * in all states. So we pretty much need to flush both ports internally,
     * but client should only get the FLUSH_(INPUT|OUTPUT)_DONE for the one it
     * requested.  Since OMX_COMPONENT_(OUTPUT|INPUT)_FLUSH_PENDING isn't set,
     * we automatically omit sending the FLUSH done for the "opposite" port. */

    input_flush_progress = true;
    output_flush_progress = true;
    bRet = execute_flush_all();
#else
    if (flushType == 0 || flushType == OMX_ALL) {
        input_flush_progress = true;
        //flush input only
        bRet = execute_input_flush();
    }
    if (flushType == 1 || flushType == OMX_ALL) {
        //flush output only
        output_flush_progress = true;
        bRet = execute_output_flush();
    }
#endif
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
bool omx_video::execute_output_flush(void)
{
    unsigned long p1 = 0; // Parameter - 1
    unsigned long p2 = 0; // Parameter - 2
    unsigned long ident = 0;
    bool bRet = true;

    /*Generate FBD for all Buffers in the FTBq*/
    DEBUG_PRINT_LOW("execute_output_flush");
    pthread_mutex_lock(&m_lock);
    while (m_ftb_q.m_size) {
        m_ftb_q.pop_entry(&p1,&p2,&ident);

        if (ident == OMX_COMPONENT_GENERATE_FTB ) {
            pending_output_buffers++;
            fill_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p2);
        } else if (ident == OMX_COMPONENT_GENERATE_FBD) {
            fill_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p1);
        }
    }

    pthread_mutex_unlock(&m_lock);
    /*Check if there are buffers with the Driver*/
    if (dev_flush(PORT_INDEX_OUT)) {
        DEBUG_PRINT_ERROR("ERROR: o/p dev_flush() Failed");
        return false;
    }

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
bool omx_video::execute_input_flush(void)
{
    unsigned long p1 = 0; // Parameter - 1
    unsigned long p2 = 0; // Parameter - 2
    unsigned long ident = 0;
    bool bRet = true;

    /*Generate EBD for all Buffers in the ETBq*/
    DEBUG_PRINT_LOW("execute_input_flush");

    pthread_mutex_lock(&m_lock);
    while (m_etb_q.m_size) {
        m_etb_q.pop_entry(&p1,&p2,&ident);
        if (ident == OMX_COMPONENT_GENERATE_ETB) {
            pending_input_buffers++;
            empty_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p2);
        } else if (ident == OMX_COMPONENT_GENERATE_EBD) {
            empty_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p1);
        } else if (ident == OMX_COMPONENT_GENERATE_ETB_OPQ) {
            m_pCallbacks.EmptyBufferDone(&m_cmp,m_app_data,(OMX_BUFFERHEADERTYPE *)p2);
        }
    }
    if (mUseProxyColorFormat) {
        if (psource_frame) {
            m_pCallbacks.EmptyBufferDone(&m_cmp,m_app_data,psource_frame);
            psource_frame = NULL;
        }
        while (m_opq_meta_q.m_size) {
            unsigned long p1,p2,id;
            m_opq_meta_q.pop_entry(&p1,&p2,&id);
            m_pCallbacks.EmptyBufferDone(&m_cmp,m_app_data,
                    (OMX_BUFFERHEADERTYPE  *)p1);
        }
        if (pdest_frame) {
            m_opq_pmem_q.insert_entry((unsigned long)pdest_frame,0,0);
            pdest_frame = NULL;
        }
    }
    pthread_mutex_unlock(&m_lock);
    /*Check if there are buffers with the Driver*/
    if (dev_flush(PORT_INDEX_IN)) {
        DEBUG_PRINT_ERROR("ERROR: i/p dev_flush() Failed");
        return false;
    }

    return bRet;
}


/*=========================================================================
FUNCTION : execute_flush

DESCRIPTION
Executes the OMX flush at INPUT & OUTPUT PORT.

PARAMETERS
None.

RETURN VALUE
true/false
==========================================================================*/
#ifdef _MSM8974_
bool omx_video::execute_flush_all(void)
{
    unsigned long p1 = 0; // Parameter - 1
    unsigned long p2 = 0; // Parameter - 2
    unsigned long ident = 0;
    bool bRet = true;

    DEBUG_PRINT_LOW("execute_flush_all");

    /*Generate EBD for all Buffers in the ETBq*/
    pthread_mutex_lock(&m_lock);
    while (m_etb_q.m_size) {
        m_etb_q.pop_entry(&p1,&p2,&ident);
        if (ident == OMX_COMPONENT_GENERATE_ETB) {
            pending_input_buffers++;
            empty_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p2);
        } else if (ident == OMX_COMPONENT_GENERATE_EBD) {
            empty_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p1);
        } else if(ident == OMX_COMPONENT_GENERATE_ETB_OPQ) {
            m_pCallbacks.EmptyBufferDone(&m_cmp,m_app_data,(OMX_BUFFERHEADERTYPE *)p2);
        }
    }
    if(mUseProxyColorFormat) {
        if(psource_frame) {
            m_pCallbacks.EmptyBufferDone(&m_cmp,m_app_data,psource_frame);
            psource_frame = NULL;
        }
        while(m_opq_meta_q.m_size) {
            unsigned long p1,p2,id;
            m_opq_meta_q.pop_entry(&p1,&p2,&id);
            m_pCallbacks.EmptyBufferDone(&m_cmp,m_app_data,
                (OMX_BUFFERHEADERTYPE  *)p1);
        }
        if(pdest_frame){
            m_opq_pmem_q.insert_entry((unsigned long)pdest_frame,0,0);
            pdest_frame = NULL;
        }
    }

    /*Generate FBD for all Buffers in the FTBq*/
    DEBUG_PRINT_LOW("execute_output_flush");
    while (m_ftb_q.m_size) {
        m_ftb_q.pop_entry(&p1,&p2,&ident);

        if (ident == OMX_COMPONENT_GENERATE_FTB ) {
            pending_output_buffers++;
            fill_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p2);
        } else if (ident == OMX_COMPONENT_GENERATE_FBD) {
            fill_buffer_done(&m_cmp,(OMX_BUFFERHEADERTYPE *)p1);
        }
    }

    pthread_mutex_unlock(&m_lock);

    /*Check if there are buffers with the Driver*/
    if (dev_flush(PORT_INDEX_BOTH)) {
        DEBUG_PRINT_ERROR("ERROR: dev_flush() Failed");
        return false;
    }
    return bRet;
}

#endif

/* ======================================================================
   FUNCTION
   omx_venc::SendCommandEvent

   DESCRIPTION
   Send the event to decoder pipe.  This is needed to generate the callbacks
   in decoder thread context.

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
bool omx_video::post_event(unsigned long p1,
        unsigned long p2,
        unsigned long id)
{
    bool bRet =  false;

    pthread_mutex_lock(&m_lock);

    if ((id == OMX_COMPONENT_GENERATE_FTB) ||
            (id == OMX_COMPONENT_GENERATE_FBD) ||
            (id == OMX_COMPONENT_GENERATE_EVENT_OUTPUT_FLUSH)) {
        m_ftb_q.insert_entry(p1,p2,id);
    } else if ((id == OMX_COMPONENT_GENERATE_ETB) ||
            (id == OMX_COMPONENT_GENERATE_EBD) ||
            (id == OMX_COMPONENT_GENERATE_EVENT_INPUT_FLUSH)) {
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

/* ======================================================================
   FUNCTION
   omx_venc::GetParameter

   DESCRIPTION
   OMX Get Parameter method implementation

   PARAMETERS
   <TBD>.

   RETURN VALUE
   Error None if successful.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
        OMX_IN OMX_INDEXTYPE paramIndex,
        OMX_INOUT OMX_PTR     paramData)
{
    (void)hComp;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    unsigned int height=0,width = 0;

    DEBUG_PRINT_LOW("get_parameter:");
    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("ERROR: Get Param in Invalid State");
        return OMX_ErrorInvalidState;
    }
    if (paramData == NULL) {
        DEBUG_PRINT_ERROR("ERROR: Get Param in Invalid paramData");
        return OMX_ErrorBadParameter;
    }
    switch ((int)paramIndex) {
        case OMX_IndexParamPortDefinition:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_PARAM_PORTDEFINITIONTYPE);
                OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
                portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamPortDefinition");
                if (portDefn->nPortIndex == (OMX_U32) PORT_INDEX_IN) {
                    dev_get_buf_req (&m_sInPortDef.nBufferCountMin,
                        &m_sInPortDef.nBufferCountActual,
                        &m_sInPortDef.nBufferSize,
                        m_sInPortDef.nPortIndex);
                    DEBUG_PRINT_LOW("m_sInPortDef: size = %u, min cnt = %u, actual cnt = %u",
                            (unsigned int)m_sInPortDef.nBufferSize, (unsigned int)m_sInPortDef.nBufferCountMin,
                            (unsigned int)m_sInPortDef.nBufferCountActual);
                    memcpy(portDefn, &m_sInPortDef, sizeof(m_sInPortDef));
#ifdef _ANDROID_ICS_
                    if (meta_mode_enable) {
                        portDefn->nBufferSize = sizeof(encoder_media_buffer_type);
                    }
                    if (mUseProxyColorFormat) {
                        portDefn->format.video.eColorFormat =
                            (OMX_COLOR_FORMATTYPE)QOMX_COLOR_FormatAndroidOpaque;
                    }
#endif
                } else if (portDefn->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    dev_get_buf_req (&m_sOutPortDef.nBufferCountMin,
                            &m_sOutPortDef.nBufferCountActual,
                            &m_sOutPortDef.nBufferSize,
                            m_sOutPortDef.nPortIndex);
                    DEBUG_PRINT_LOW("m_sOutPortDef: size = %u, min cnt = %u, actual cnt = %u",
                            (unsigned int)m_sOutPortDef.nBufferSize, (unsigned int)m_sOutPortDef.nBufferCountMin,
                            (unsigned int)m_sOutPortDef.nBufferCountActual);
                    memcpy(portDefn, &m_sOutPortDef, sizeof(m_sOutPortDef));
                } else {
                    DEBUG_PRINT_ERROR("ERROR: GetParameter called on Bad Port Index");
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }
        case OMX_IndexParamVideoInit:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_PORT_PARAM_TYPE);
                OMX_PORT_PARAM_TYPE *portParamType =
                    (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoInit");

                memcpy(portParamType, &m_sPortParam, sizeof(m_sPortParam));
                break;
            }
        case OMX_IndexParamVideoPortFormat:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_PORTFORMATTYPE);
                OMX_VIDEO_PARAM_PORTFORMATTYPE *portFmt =
                    (OMX_VIDEO_PARAM_PORTFORMATTYPE *)paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoPortFormat");

                if (portFmt->nPortIndex == (OMX_U32) PORT_INDEX_IN) {
                    unsigned index = portFmt->nIndex;
                    //we support two formats
                    //index 0 - Venus flavour of YUV420SP
                    //index 1 - opaque which internally maps to YUV420SP.
                    //index 2 - vannilla YUV420SP
                    //this can be extended in the future
                    int supportedFormats[] = {
                        [0] = QOMX_COLOR_FORMATYUV420PackedSemiPlanar32m,
                        [1] = QOMX_COLOR_FormatAndroidOpaque,
                        [2] = OMX_COLOR_FormatYUV420SemiPlanar,
                    };

                    if (index >= sizeof(supportedFormats)/sizeof(*supportedFormats))
                        eRet = OMX_ErrorNoMore;
                    else {
                        memcpy(portFmt, &m_sInPortFormat, sizeof(m_sInPortFormat));
                        portFmt->nIndex = index; //restore index set from client
                        portFmt->eColorFormat = (OMX_COLOR_FORMATTYPE)supportedFormats[index];
                    }
                } else if (portFmt->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    memcpy(portFmt, &m_sOutPortFormat, sizeof(m_sOutPortFormat));
                } else {
                    DEBUG_PRINT_ERROR("ERROR: GetParameter called on Bad Port Index");
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }
        case OMX_IndexParamVideoBitrate:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_BITRATETYPE);
                OMX_VIDEO_PARAM_BITRATETYPE* pParam = (OMX_VIDEO_PARAM_BITRATETYPE*)paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoBitrate");

                if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    memcpy(pParam, &m_sParamBitrate, sizeof(m_sParamBitrate));
                } else {
                    DEBUG_PRINT_ERROR("ERROR: GetParameter called on Bad Port Index");
                    eRet = OMX_ErrorBadPortIndex;
                }

                break;
            }
        case OMX_IndexParamVideoMpeg4:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_MPEG4TYPE);
                OMX_VIDEO_PARAM_MPEG4TYPE* pParam = (OMX_VIDEO_PARAM_MPEG4TYPE*)paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoMpeg4");
                memcpy(pParam, &m_sParamMPEG4, sizeof(m_sParamMPEG4));
                break;
            }
        case OMX_IndexParamVideoH263:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_H263TYPE);
                OMX_VIDEO_PARAM_H263TYPE* pParam = (OMX_VIDEO_PARAM_H263TYPE*)paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoH263");
                memcpy(pParam, &m_sParamH263, sizeof(m_sParamH263));
                break;
            }
        case OMX_IndexParamVideoAvc:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_AVCTYPE);
                OMX_VIDEO_PARAM_AVCTYPE* pParam = (OMX_VIDEO_PARAM_AVCTYPE*)paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoAvc");
                memcpy(pParam, &m_sParamAVC, sizeof(m_sParamAVC));
                break;
            }
        case (OMX_INDEXTYPE)OMX_IndexParamVideoVp8:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_VP8TYPE);
                OMX_VIDEO_PARAM_VP8TYPE* pParam = (OMX_VIDEO_PARAM_VP8TYPE*)paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoVp8");
                memcpy(pParam, &m_sParamVP8, sizeof(m_sParamVP8));
                break;
            }
        case OMX_IndexParamVideoProfileLevelQuerySupported:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_PROFILELEVELTYPE);
                OMX_VIDEO_PARAM_PROFILELEVELTYPE* pParam = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported");
                eRet = get_supported_profile_level(pParam);
                if (eRet && eRet != OMX_ErrorNoMore)
                    DEBUG_PRINT_ERROR("Invalid entry returned from get_supported_profile_level %u, %u",
                            (unsigned int)pParam->eProfile, (unsigned int)pParam->eLevel);
                break;
            }
        case OMX_IndexParamVideoProfileLevelCurrent:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_PROFILELEVELTYPE);
                OMX_VIDEO_PARAM_PROFILELEVELTYPE* pParam = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoProfileLevelCurrent");
                memcpy(pParam, &m_sParamProfileLevel, sizeof(m_sParamProfileLevel));
                break;
            }
            /*Component should support this port definition*/
        case OMX_IndexParamAudioInit:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_PORT_PARAM_TYPE);
                OMX_PORT_PARAM_TYPE *audioPortParamType = (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamAudioInit");
                memcpy(audioPortParamType, &m_sPortParam_audio, sizeof(m_sPortParam_audio));
                break;
            }
            /*Component should support this port definition*/
        case OMX_IndexParamImageInit:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_PORT_PARAM_TYPE);
                OMX_PORT_PARAM_TYPE *imagePortParamType = (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamImageInit");
                memcpy(imagePortParamType, &m_sPortParam_img, sizeof(m_sPortParam_img));
                break;

            }
            /*Component should support this port definition*/
        case OMX_IndexParamOtherInit:
            {
                DEBUG_PRINT_ERROR("ERROR: get_parameter: OMX_IndexParamOtherInit %08x", paramIndex);
                eRet =OMX_ErrorUnsupportedIndex;
                break;
            }
        case OMX_IndexParamStandardComponentRole:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_PARAM_COMPONENTROLETYPE);
                OMX_PARAM_COMPONENTROLETYPE *comp_role;
                comp_role = (OMX_PARAM_COMPONENTROLETYPE *) paramData;
                comp_role->nVersion.nVersion = OMX_SPEC_VERSION;
                comp_role->nSize = sizeof(*comp_role);

                DEBUG_PRINT_LOW("Getparameter: OMX_IndexParamStandardComponentRole %d",paramIndex);
                if (NULL != comp_role->cRole) {
                    strlcpy((char*)comp_role->cRole,(const char*)m_cRole,OMX_MAX_STRINGNAME_SIZE);
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Getparameter: OMX_IndexParamStandardComponentRole %d is passed with NULL parameter for role",paramIndex);
                    eRet =OMX_ErrorBadParameter;
                }
                break;
            }
            /* Added for parameter test */
        case OMX_IndexParamPriorityMgmt:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_PRIORITYMGMTTYPE);
                OMX_PRIORITYMGMTTYPE *priorityMgmType = (OMX_PRIORITYMGMTTYPE *) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamPriorityMgmt");
                memcpy(priorityMgmType, &m_sPriorityMgmt, sizeof(m_sPriorityMgmt));
                break;
            }
            /* Added for parameter test */
        case OMX_IndexParamCompBufferSupplier:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_PARAM_BUFFERSUPPLIERTYPE);
                OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamCompBufferSupplier");
                if (bufferSupplierType->nPortIndex ==(OMX_U32) PORT_INDEX_IN) {
                    memcpy(bufferSupplierType, &m_sInBufSupplier, sizeof(m_sInBufSupplier));
                } else if (bufferSupplierType->nPortIndex ==(OMX_U32) PORT_INDEX_OUT) {
                    memcpy(bufferSupplierType, &m_sOutBufSupplier, sizeof(m_sOutBufSupplier));
                } else {
                    DEBUG_PRINT_ERROR("ERROR: GetParameter called on Bad Port Index");
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }

        case OMX_IndexParamVideoQuantization:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_QUANTIZATIONTYPE);
                OMX_VIDEO_PARAM_QUANTIZATIONTYPE *session_qp = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE*) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoQuantization");
                memcpy(session_qp, &m_sSessionQuantization, sizeof(m_sSessionQuantization));
                break;
            }

        case OMX_QcomIndexParamVideoQPRange:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_QCOM_VIDEO_PARAM_QPRANGETYPE);
                OMX_QCOM_VIDEO_PARAM_QPRANGETYPE *qp_range = (OMX_QCOM_VIDEO_PARAM_QPRANGETYPE*) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_QcomIndexParamVideoQPRange");
                memcpy(qp_range, &m_sSessionQPRange, sizeof(m_sSessionQPRange));
                break;
            }

        case OMX_IndexParamVideoErrorCorrection:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE);
                OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* errorresilience = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE*)paramData;
                DEBUG_PRINT_LOW("OMX_IndexParamVideoErrorCorrection");
                errorresilience->bEnableHEC = m_sErrorCorrection.bEnableHEC;
                errorresilience->bEnableResync = m_sErrorCorrection.bEnableResync;
                errorresilience->nResynchMarkerSpacing = m_sErrorCorrection.nResynchMarkerSpacing;
                break;
            }
        case OMX_IndexParamVideoIntraRefresh:
            {
                 VALIDATE_OMX_PARAM_DATA(paramData, OMX_VIDEO_PARAM_INTRAREFRESHTYPE);
                OMX_VIDEO_PARAM_INTRAREFRESHTYPE* intrarefresh = (OMX_VIDEO_PARAM_INTRAREFRESHTYPE*)paramData;
                DEBUG_PRINT_LOW("OMX_IndexParamVideoIntraRefresh");
                DEBUG_PRINT_ERROR("OMX_IndexParamVideoIntraRefresh GET");
                intrarefresh->eRefreshMode = m_sIntraRefresh.eRefreshMode;
                intrarefresh->nCirMBs = m_sIntraRefresh.nCirMBs;
                break;
            }
        case OMX_QcomIndexPortDefn:
            //TODO
            break;
        case OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMXComponentCapabilityFlagsType);
                OMXComponentCapabilityFlagsType *pParam = reinterpret_cast<OMXComponentCapabilityFlagsType*>(paramData);
                DEBUG_PRINT_LOW("get_parameter: OMX_COMPONENT_CAPABILITY_TYPE_INDEX");
                pParam->iIsOMXComponentMultiThreaded = OMX_TRUE;
                pParam->iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE;
                pParam->iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
                pParam->iOMXComponentSupportsMovableInputBuffers = OMX_TRUE;
                pParam->iOMXComponentUsesNALStartCodes = OMX_TRUE;
                pParam->iOMXComponentSupportsPartialFrames = OMX_FALSE;
                pParam->iOMXComponentCanHandleIncompleteFrames = OMX_FALSE;
                pParam->iOMXComponentUsesFullAVCFrames = OMX_FALSE;
                m_use_input_pmem = OMX_TRUE;
                DEBUG_PRINT_LOW("Supporting capability index in encoder node");
                break;
            }
#if !defined(MAX_RES_720P) || defined(_MSM8974_)
        case OMX_QcomIndexParamIndexExtraDataType:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, QOMX_INDEXEXTRADATATYPE);
                DEBUG_PRINT_LOW("get_parameter: OMX_QcomIndexParamIndexExtraDataType");
                QOMX_INDEXEXTRADATATYPE *pParam = (QOMX_INDEXEXTRADATATYPE *)paramData;
                if (pParam->nIndex == (OMX_INDEXTYPE)OMX_ExtraDataVideoEncoderSliceInfo) {
                    if (pParam->nPortIndex == PORT_INDEX_OUT) {
                        pParam->bEnabled =
                            (OMX_BOOL)(m_sExtraData & VEN_EXTRADATA_SLICEINFO);
                        DEBUG_PRINT_HIGH("Slice Info extradata %d", pParam->bEnabled);
                    } else {
                        DEBUG_PRINT_ERROR("get_parameter: slice information is "
                                "valid for output port only");
                        eRet =OMX_ErrorUnsupportedIndex;
                    }
                } else if (pParam->nIndex == (OMX_INDEXTYPE)OMX_ExtraDataVideoEncoderMBInfo) {
                    if (pParam->nPortIndex == PORT_INDEX_OUT) {
                        pParam->bEnabled =
                            (OMX_BOOL)(m_sExtraData & VEN_EXTRADATA_MBINFO);
                        DEBUG_PRINT_HIGH("MB Info extradata %d", pParam->bEnabled);
                    } else {
                        DEBUG_PRINT_ERROR("get_parameter: MB information is "
                                "valid for output port only");
                        eRet = OMX_ErrorUnsupportedIndex;
                    }
                }
#ifndef _MSM8974_
                else if (pParam->nIndex == (OMX_INDEXTYPE)OMX_ExtraDataVideoLTRInfo) {
                    if (pParam->nPortIndex == PORT_INDEX_OUT) {
                        pParam->bEnabled =
                            (OMX_BOOL)(m_sExtraData & VEN_EXTRADATA_LTRINFO);
                        DEBUG_PRINT_HIGH("LTR Info extradata %d", pParam->bEnabled);
                    } else {
                        DEBUG_PRINT_ERROR("get_parameter: LTR information is "
                                "valid for output port only");
                        eRet = OMX_ErrorUnsupportedIndex;
                    }
                }
#endif
                else {
                    DEBUG_PRINT_ERROR("get_parameter: unsupported extradata index (0x%x)",
                            pParam->nIndex);
                    eRet = OMX_ErrorUnsupportedIndex;
                }
                break;
            }
        case QOMX_IndexParamVideoLTRCountRangeSupported:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, QOMX_EXTNINDEX_RANGETYPE);
                DEBUG_PRINT_HIGH("get_parameter: QOMX_IndexParamVideoLTRCountRangeSupported");
                QOMX_EXTNINDEX_RANGETYPE *pParam = (QOMX_EXTNINDEX_RANGETYPE *)paramData;
                if (pParam->nPortIndex == PORT_INDEX_OUT) {
                    OMX_U32 min = 0, max = 0, step_size = 0;
                    if (dev_get_capability_ltrcount(&min, &max, &step_size)) {
                        pParam->nMin = min;
                        pParam->nMax = max;
                        pParam->nStepSize = step_size;
                    } else {
                        DEBUG_PRINT_ERROR("get_parameter: get_capability_ltrcount failed");
                        eRet = OMX_ErrorUndefined;
                    }
                } else {
                    DEBUG_PRINT_ERROR("LTR count range is valid for output port only");
                    eRet = OMX_ErrorUnsupportedIndex;
                }
            }
            break;
#endif
        case QOMX_IndexParamVideoSyntaxHdr:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, QOMX_EXTNINDEX_PARAMTYPE);
                DEBUG_PRINT_HIGH("QOMX_IndexParamVideoSyntaxHdr");
                QOMX_EXTNINDEX_PARAMTYPE* pParam =
                    reinterpret_cast<QOMX_EXTNINDEX_PARAMTYPE*>(paramData);
                if (pParam->pData == NULL) {
                    DEBUG_PRINT_ERROR("Error: Data buffer is NULL");
                    eRet = OMX_ErrorBadParameter;
                    break;
                }
                if (get_syntaxhdr_enable == false) {
                    DEBUG_PRINT_ERROR("ERROR: get_parameter: Get syntax header disabled");
                    eRet = OMX_ErrorUnsupportedIndex;
                    break;
                }
                BITMASK_SET(&m_flags, OMX_COMPONENT_LOADED_START_PENDING);
                if (dev_loaded_start()) {
                    DEBUG_PRINT_LOW("device start successful");
                } else {
                    DEBUG_PRINT_ERROR("device start failed");
                    BITMASK_CLEAR(&m_flags, OMX_COMPONENT_LOADED_START_PENDING);
                    return OMX_ErrorHardware;
                }
                if (dev_get_seq_hdr(pParam->pData,
                            (unsigned)(pParam->nSize - sizeof(QOMX_EXTNINDEX_PARAMTYPE)),
                            (OMX_U32 *)(void *)&pParam->nDataSize)) {
                    DEBUG_PRINT_HIGH("get syntax header successful (hdrlen = %u)",
                            (unsigned int)pParam->nDataSize);
                    for (unsigned i = 0; i < pParam->nDataSize; i++) {
                        DEBUG_PRINT_LOW("Header[%d] = %x", i, *((char *)pParam->pData + i));
                    }
                } else {
                    DEBUG_PRINT_ERROR("Error returned from GetSyntaxHeader()");
                    eRet = OMX_ErrorHardware;
                }
                BITMASK_SET(&m_flags, OMX_COMPONENT_LOADED_STOP_PENDING);
                if (dev_loaded_stop()) {
                    DEBUG_PRINT_LOW("device stop successful");
                } else {
                    DEBUG_PRINT_ERROR("device stop failed");
                    BITMASK_CLEAR(&m_flags, OMX_COMPONENT_LOADED_STOP_PENDING);
                    eRet = OMX_ErrorHardware;
                }
                break;
            }
        case OMX_QcomIndexHierarchicalStructure:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, QOMX_VIDEO_HIERARCHICALLAYERS);
                QOMX_VIDEO_HIERARCHICALLAYERS* hierp = (QOMX_VIDEO_HIERARCHICALLAYERS*) paramData;
                DEBUG_PRINT_LOW("get_parameter: OMX_QcomIndexHierarchicalStructure");
                memcpy(hierp, &m_sHierLayers, sizeof(m_sHierLayers));
                break;
            }
        case OMX_QcomIndexParamPerfLevel:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_QCOM_VIDEO_PARAM_PERF_LEVEL);
                OMX_U32 perflevel;
                OMX_QCOM_VIDEO_PARAM_PERF_LEVEL *pParam =
                    reinterpret_cast<OMX_QCOM_VIDEO_PARAM_PERF_LEVEL*>(paramData);
                DEBUG_PRINT_LOW("get_parameter: OMX_QcomIndexParamPerfLevel");
                if (!dev_get_performance_level(&perflevel)) {
                    DEBUG_PRINT_ERROR("Invalid entry returned from get_performance_level %d",
                        pParam->ePerfLevel);
                } else {
                    pParam->ePerfLevel = (QOMX_VIDEO_PERF_LEVEL)perflevel;
                }
                break;
            }
        case OMX_QcomIndexParamH264VUITimingInfo:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_QCOM_VIDEO_PARAM_VUI_TIMING_INFO);
                OMX_U32 enabled;
                OMX_QCOM_VIDEO_PARAM_VUI_TIMING_INFO *pParam =
                    reinterpret_cast<OMX_QCOM_VIDEO_PARAM_VUI_TIMING_INFO*>(paramData);
                DEBUG_PRINT_LOW("get_parameter: OMX_QcomIndexParamH264VUITimingInfo");
                if (!dev_get_vui_timing_info(&enabled)) {
                    DEBUG_PRINT_ERROR("Invalid entry returned from get_vui_Timing_info %d",
                        pParam->bEnable);
                } else {
                    pParam->bEnable = (OMX_BOOL)enabled;
                }
                break;
            }
        case OMX_QcomIndexParamPeakBitrate:
            {
                VALIDATE_OMX_PARAM_DATA(paramData, OMX_QCOM_VIDEO_PARAM_PEAK_BITRATE);
                OMX_U32 peakbitrate;
                OMX_QCOM_VIDEO_PARAM_PEAK_BITRATE *pParam =
                    reinterpret_cast<OMX_QCOM_VIDEO_PARAM_PEAK_BITRATE*>(paramData);
                DEBUG_PRINT_LOW("get_parameter: OMX_QcomIndexParamPeakBitrate");
                if (!dev_get_peak_bitrate(&peakbitrate)) {
                    DEBUG_PRINT_ERROR("Invalid entry returned from get_peak_bitrate %u",
                        (unsigned int)pParam->nPeakBitrate);
                } else {
                    pParam->nPeakBitrate = peakbitrate;
                }
                break;
            }
         case QOMX_IndexParamVideoInitialQp:
            {
                 VALIDATE_OMX_PARAM_DATA(paramData, QOMX_EXTNINDEX_VIDEO_INITIALQP);
                 QOMX_EXTNINDEX_VIDEO_INITIALQP* initqp =
                     reinterpret_cast<QOMX_EXTNINDEX_VIDEO_INITIALQP *>(paramData);
                     memcpy(initqp, &m_sParamInitqp, sizeof(m_sParamInitqp));
                break;
            }
        case OMX_IndexParamVideoSliceFMO:
        default:
            {
                DEBUG_PRINT_LOW("ERROR: get_parameter: unknown param %08x", paramIndex);
                eRet =OMX_ErrorUnsupportedIndex;
                break;
            }

    }

    return eRet;

}
/* ======================================================================
   FUNCTION
   omx_video::GetConfig

   DESCRIPTION
   OMX Get Config Method implementation.

   PARAMETERS
   <TBD>.

   RETURN VALUE
   OMX Error None if successful.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::get_config(OMX_IN OMX_HANDLETYPE      hComp,
        OMX_IN OMX_INDEXTYPE configIndex,
        OMX_INOUT OMX_PTR     configData)
{
    (void)hComp;
    ////////////////////////////////////////////////////////////////
    // Supported Config Index           Type
    // =============================================================
    // OMX_IndexConfigVideoBitrate      OMX_VIDEO_CONFIG_BITRATETYPE
    // OMX_IndexConfigVideoFramerate    OMX_CONFIG_FRAMERATETYPE
    // OMX_IndexConfigCommonRotate      OMX_CONFIG_ROTATIONTYPE
    ////////////////////////////////////////////////////////////////

    if (configData == NULL) {
        DEBUG_PRINT_ERROR("ERROR: param is null");
        return OMX_ErrorBadParameter;
    }

    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("ERROR: can't be in invalid state");
        return OMX_ErrorIncorrectStateOperation;
    }

    //@todo need to validate params
    switch ((int)configIndex) {
        case OMX_IndexConfigVideoBitrate:
            {
                VALIDATE_OMX_PARAM_DATA(configData, OMX_VIDEO_CONFIG_BITRATETYPE);
                OMX_VIDEO_CONFIG_BITRATETYPE* pParam = reinterpret_cast<OMX_VIDEO_CONFIG_BITRATETYPE*>(configData);
                memcpy(pParam, &m_sConfigBitrate, sizeof(m_sConfigBitrate));
                break;
            }
        case OMX_IndexConfigVideoFramerate:
            {
                VALIDATE_OMX_PARAM_DATA(configData, OMX_CONFIG_FRAMERATETYPE);
                OMX_CONFIG_FRAMERATETYPE* pParam = reinterpret_cast<OMX_CONFIG_FRAMERATETYPE*>(configData);
                memcpy(pParam, &m_sConfigFramerate, sizeof(m_sConfigFramerate));
                break;
            }
        case OMX_IndexConfigCommonRotate:
            {
                VALIDATE_OMX_PARAM_DATA(configData, OMX_CONFIG_ROTATIONTYPE);
                OMX_CONFIG_ROTATIONTYPE* pParam = reinterpret_cast<OMX_CONFIG_ROTATIONTYPE*>(configData);
                memcpy(pParam, &m_sConfigFrameRotation, sizeof(m_sConfigFrameRotation));
                break;
            }
        case QOMX_IndexConfigVideoIntraperiod:
            {
                DEBUG_PRINT_LOW("get_config:QOMX_IndexConfigVideoIntraperiod");
                QOMX_VIDEO_INTRAPERIODTYPE* pParam = reinterpret_cast<QOMX_VIDEO_INTRAPERIODTYPE*>(configData);
                memcpy(pParam, &m_sIntraperiod, sizeof(m_sIntraperiod));
                break;
            }
        case OMX_IndexConfigVideoAVCIntraPeriod:
            {
                VALIDATE_OMX_PARAM_DATA(configData, QOMX_VIDEO_INTRAPERIODTYPE);
                OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pParam =
                    reinterpret_cast<OMX_VIDEO_CONFIG_AVCINTRAPERIOD*>(configData);
                DEBUG_PRINT_LOW("get_config: OMX_IndexConfigVideoAVCIntraPeriod");
                memcpy(pParam, &m_sConfigAVCIDRPeriod, sizeof(m_sConfigAVCIDRPeriod));
                break;
            }
        case OMX_IndexConfigCommonDeinterlace:
            {
                VALIDATE_OMX_PARAM_DATA(configData, OMX_VIDEO_CONFIG_DEINTERLACE);
                OMX_VIDEO_CONFIG_DEINTERLACE *pParam =
                    reinterpret_cast<OMX_VIDEO_CONFIG_DEINTERLACE*>(configData);
                DEBUG_PRINT_LOW("get_config: OMX_IndexConfigCommonDeinterlace");
                memcpy(pParam, &m_sConfigDeinterlace, sizeof(m_sConfigDeinterlace));
                break;
            }
       case OMX_IndexConfigVideoVp8ReferenceFrame:
           {
               VALIDATE_OMX_PARAM_DATA(configData, OMX_VIDEO_VP8REFERENCEFRAMETYPE);
               OMX_VIDEO_VP8REFERENCEFRAMETYPE* pParam =
                   reinterpret_cast<OMX_VIDEO_VP8REFERENCEFRAMETYPE*>(configData);
               DEBUG_PRINT_LOW("get_config: OMX_IndexConfigVideoVp8ReferenceFrame");
               memcpy(pParam, &m_sConfigVp8ReferenceFrame, sizeof(m_sConfigVp8ReferenceFrame));
               break;
           }
        default:
            DEBUG_PRINT_ERROR("ERROR: unsupported index %d", (int) configIndex);
            return OMX_ErrorUnsupportedIndex;
    }
    return OMX_ErrorNone;

}

/* ======================================================================
   FUNCTION
   omx_video::GetExtensionIndex

   DESCRIPTION
   OMX GetExtensionIndex method implementaion.  <TBD>

   PARAMETERS
   <TBD>.

   RETURN VALUE
   OMX Error None if everything successful.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::get_extension_index(OMX_IN OMX_HANDLETYPE      hComp,
        OMX_IN OMX_STRING      paramName,
        OMX_OUT OMX_INDEXTYPE* indexType)
{
    (void)hComp;
    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("ERROR: Get Extension Index in Invalid State");
        return OMX_ErrorInvalidState;
    }
#ifdef MAX_RES_1080P
    if (!strncmp(paramName, "OMX.QCOM.index.param.SliceDeliveryMode",
                sizeof("OMX.QCOM.index.param.SliceDeliveryMode") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_QcomIndexEnableSliceDeliveryMode;
        return OMX_ErrorNone;
    }
#endif
#ifdef _ANDROID_ICS_
    if (!strncmp(paramName, "OMX.google.android.index.storeMetaDataInBuffers",
            sizeof("OMX.google.android.index.storeMetaDataInBuffers") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_QcomIndexParamVideoMetaBufferMode;
        return OMX_ErrorNone;
    }
#endif
    if (!strncmp(paramName, "OMX.google.android.index.prependSPSPPSToIDRFrames",
            sizeof("OMX.google.android.index.prependSPSPPSToIDRFrames") - 1)) {
        *indexType = (OMX_INDEXTYPE)OMX_QcomIndexParamSequenceHeaderWithIDR;
        return OMX_ErrorNone;
    }
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
   FUNCTION
   omx_video::GetState

   DESCRIPTION
   Returns the state information back to the caller.<TBD>

   PARAMETERS
   <TBD>.

   RETURN VALUE
   Error None if everything is successful.
   ========================================================================== */
OMX_ERRORTYPE  omx_video::get_state(OMX_IN OMX_HANDLETYPE  hComp,
        OMX_OUT OMX_STATETYPE* state)
{
    (void)hComp;
    *state = m_state;
    DEBUG_PRINT_LOW("get_state: Returning the state %d",*state);
    return OMX_ErrorNone;
}

/* ======================================================================
   FUNCTION
   omx_video::ComponentTunnelRequest

   DESCRIPTION
   OMX Component Tunnel Request method implementation. <TBD>

   PARAMETERS
   None.

   RETURN VALUE
   OMX Error None if everything successful.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::component_tunnel_request(OMX_IN OMX_HANDLETYPE  hComp,
        OMX_IN OMX_U32                        port,
        OMX_IN OMX_HANDLETYPE        peerComponent,
        OMX_IN OMX_U32                    peerPort,
        OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup)
{
    (void) hComp, (void) port, (void) peerComponent, (void) peerPort, (void) tunnelSetup;
    DEBUG_PRINT_ERROR("ERROR: component_tunnel_request Not Implemented");
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
   FUNCTION
   omx_video::UseInputBuffer

   DESCRIPTION
   Helper function for Use buffer in the input pin

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
OMX_ERRORTYPE  omx_video::use_input_buffer(
        OMX_IN OMX_HANDLETYPE            hComp,
        OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
        OMX_IN OMX_U32                   port,
        OMX_IN OMX_PTR                   appData,
        OMX_IN OMX_U32                   bytes,
        OMX_IN OMX_U8*                   buffer)
{
    (void) hComp;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    unsigned   i = 0;
    unsigned char *buf_addr = NULL;

    DEBUG_PRINT_HIGH("use_input_buffer: port = %u appData = %p bytes = %u buffer = %p",(unsigned int)port,appData,(unsigned int)bytes,buffer);
    if (bytes != m_sInPortDef.nBufferSize) {
        DEBUG_PRINT_ERROR("ERROR: use_input_buffer: Size Mismatch!! "
                "bytes[%u] != Port.nBufferSize[%u]", (unsigned int)bytes, (unsigned int)m_sInPortDef.nBufferSize);
        return OMX_ErrorBadParameter;
    }

    if (!m_inp_mem_ptr) {
        input_use_buffer = true;
        m_inp_mem_ptr = (OMX_BUFFERHEADERTYPE*) \
                        calloc( (sizeof(OMX_BUFFERHEADERTYPE)), m_sInPortDef.nBufferCountActual);
        if (m_inp_mem_ptr == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_inp_mem_ptr");
            return OMX_ErrorInsufficientResources;
        }


        m_pInput_pmem = (struct pmem *) calloc(sizeof (struct pmem), m_sInPortDef.nBufferCountActual);
        if (m_pInput_pmem == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_pInput_pmem");
            return OMX_ErrorInsufficientResources;
        }
#ifdef USE_ION
        m_pInput_ion = (struct venc_ion *) calloc(sizeof (struct venc_ion), m_sInPortDef.nBufferCountActual);
        if (m_pInput_ion == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_pInput_ion");
            return OMX_ErrorInsufficientResources;
        }
#endif

        for (i=0; i< m_sInPortDef.nBufferCountActual; i++) {
            m_pInput_pmem[i].fd = -1;
#ifdef USE_ION
            m_pInput_ion[i].ion_device_fd =-1;
            m_pInput_ion[i].fd_ion_data.fd =-1;
            m_pInput_ion[i].ion_alloc_data.handle = 0;
#endif
        }

    }

    for (i=0; i< m_sInPortDef.nBufferCountActual; i++) {
        if (BITMASK_ABSENT(&m_inp_bm_count,i)) {
            break;
        }
    }

    if (i < m_sInPortDef.nBufferCountActual) {

        *bufferHdr = (m_inp_mem_ptr + i);
        BITMASK_SET(&m_inp_bm_count,i);

        (*bufferHdr)->pBuffer           = (OMX_U8 *)buffer;
        (*bufferHdr)->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
        (*bufferHdr)->nVersion.nVersion = OMX_SPEC_VERSION;
        (*bufferHdr)->nAllocLen         = m_sInPortDef.nBufferSize;
        (*bufferHdr)->pAppPrivate       = appData;
        (*bufferHdr)->nInputPortIndex   = PORT_INDEX_IN;

        if (!m_use_input_pmem) {
#ifdef USE_ION
#ifdef _MSM8974_
            m_pInput_ion[i].ion_device_fd = alloc_map_ion_memory(m_sInPortDef.nBufferSize,
                    &m_pInput_ion[i].ion_alloc_data,
                    &m_pInput_ion[i].fd_ion_data,0);
#else
            m_pInput_ion[i].ion_device_fd = alloc_map_ion_memory(m_sInPortDef.nBufferSize,
                    &m_pInput_ion[i].ion_alloc_data,
                    &m_pInput_ion[i].fd_ion_data,ION_FLAG_CACHED);
#endif
            if (m_pInput_ion[i].ion_device_fd < 0) {
                DEBUG_PRINT_ERROR("ERROR:ION device open() Failed");
                return OMX_ErrorInsufficientResources;
            }
            m_pInput_pmem[i].fd = m_pInput_ion[i].fd_ion_data.fd;
#else
            m_pInput_pmem[i].fd = open (MEM_DEVICE,O_RDWR);
            if (m_pInput_pmem[i].fd == 0) {
                m_pInput_pmem[i].fd = open (MEM_DEVICE,O_RDWR);
            }

            if (m_pInput_pmem[i] .fd < 0) {
                DEBUG_PRINT_ERROR("ERROR: /dev/pmem_adsp open() Failed");
                return OMX_ErrorInsufficientResources;
            }
#endif
            m_pInput_pmem[i].size = m_sInPortDef.nBufferSize;
            m_pInput_pmem[i].offset = 0;

            m_pInput_pmem[i].buffer = (OMX_U8 *)SECURE_BUFPTR;
            if(!secure_session) {
                m_pInput_pmem[i].buffer = (unsigned char *)mmap(
                    NULL,m_pInput_pmem[i].size,PROT_READ|PROT_WRITE,
                    MAP_SHARED,m_pInput_pmem[i].fd,0);

            if (m_pInput_pmem[i].buffer == MAP_FAILED) {
                    DEBUG_PRINT_ERROR("ERROR: mmap() Failed");
                close(m_pInput_pmem[i].fd);
#ifdef USE_ION
                free_ion_memory(&m_pInput_ion[i]);
#endif
                return OMX_ErrorInsufficientResources;
            }
            }

        } else {
            OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pParam = reinterpret_cast<OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *>((*bufferHdr)->pAppPrivate);
            DEBUG_PRINT_LOW("Inside qcom_ext with luma:(fd:%lu,offset:0x%x)", pParam->pmem_fd, (unsigned)pParam->offset);

            if (pParam) {
                m_pInput_pmem[i].fd = pParam->pmem_fd;
                m_pInput_pmem[i].offset = pParam->offset;
                m_pInput_pmem[i].size = m_sInPortDef.nBufferSize;
                m_pInput_pmem[i].buffer = (unsigned char *)buffer;
                DEBUG_PRINT_LOW("DBG:: pParam->pmem_fd = %u, pParam->offset = %u",
                        (unsigned int)pParam->pmem_fd, (unsigned int)pParam->offset);
            } else {
                DEBUG_PRINT_ERROR("ERROR: Invalid AppData given for PMEM i/p UseBuffer case");
                return OMX_ErrorBadParameter;
            }
        }

        DEBUG_PRINT_LOW("use_inp:: bufhdr = %p, pBuffer = %p, m_pInput_pmem[i].buffer = %p",
                (*bufferHdr), (*bufferHdr)->pBuffer, m_pInput_pmem[i].buffer);
        if ( dev_use_buf(&m_pInput_pmem[i],PORT_INDEX_IN,i) != true) {
            DEBUG_PRINT_ERROR("ERROR: dev_use_buf() Failed for i/p buf");
            return OMX_ErrorInsufficientResources;
        }
    } else {
        DEBUG_PRINT_ERROR("ERROR: All buffers are already used, invalid use_buf call for "
                "index = %u", i);
        eRet = OMX_ErrorInsufficientResources;
    }

    return eRet;
}



/* ======================================================================
   FUNCTION
   omx_video::UseOutputBuffer

   DESCRIPTION
   Helper function for Use buffer in the input pin

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
OMX_ERRORTYPE  omx_video::use_output_buffer(
        OMX_IN OMX_HANDLETYPE            hComp,
        OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
        OMX_IN OMX_U32                   port,
        OMX_IN OMX_PTR                   appData,
        OMX_IN OMX_U32                   bytes,
        OMX_IN OMX_U8*                   buffer)
{
    (void)hComp, (void)port;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE       *bufHdr= NULL; // buffer header
    unsigned                         i= 0; // Temporary counter
    unsigned char *buf_addr = NULL;
#ifdef _MSM8974_
    int align_size;
#endif

    DEBUG_PRINT_HIGH("Inside use_output_buffer()");
    if (bytes != m_sOutPortDef.nBufferSize) {
        DEBUG_PRINT_ERROR("ERROR: use_output_buffer: Size Mismatch!! "
                "bytes[%u] != Port.nBufferSize[%u]", (unsigned int)bytes, (unsigned int)m_sOutPortDef.nBufferSize);
        return OMX_ErrorBadParameter;
    }

    if (!m_out_mem_ptr) {
        output_use_buffer = true;
        int nBufHdrSize        = 0;

        DEBUG_PRINT_LOW("Allocating First Output Buffer(%u)",(unsigned int)m_sOutPortDef.nBufferCountActual);
        nBufHdrSize        = m_sOutPortDef.nBufferCountActual * sizeof(OMX_BUFFERHEADERTYPE);
        /*
         * Memory for output side involves the following:
         * 1. Array of Buffer Headers
         * 2. Bitmask array to hold the buffer allocation details
         * In order to minimize the memory management entire allocation
         * is done in one step.
         */
        //OMX Buffer header
        m_out_mem_ptr = (OMX_BUFFERHEADERTYPE  *)calloc(nBufHdrSize,1);
        if (m_out_mem_ptr == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_out_mem_ptr");
            return OMX_ErrorInsufficientResources;
        }

        m_pOutput_pmem = (struct pmem *) calloc(sizeof (struct pmem), m_sOutPortDef.nBufferCountActual);
        if (m_pOutput_pmem == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_pOutput_pmem");
            return OMX_ErrorInsufficientResources;
        }
#ifdef USE_ION
        m_pOutput_ion = (struct venc_ion *) calloc(sizeof (struct venc_ion), m_sOutPortDef.nBufferCountActual);
        if (m_pOutput_ion == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_pOutput_ion");
            return OMX_ErrorInsufficientResources;
        }
#endif
        if (m_out_mem_ptr) {
            bufHdr          =  m_out_mem_ptr;
            DEBUG_PRINT_LOW("Memory Allocation Succeeded for OUT port%p",m_out_mem_ptr);
            // Settting the entire storage nicely
            for (i=0; i < m_sOutPortDef.nBufferCountActual ; i++) {
                bufHdr->nSize              = sizeof(OMX_BUFFERHEADERTYPE);
                bufHdr->nVersion.nVersion  = OMX_SPEC_VERSION;
                bufHdr->nAllocLen          = bytes;
                bufHdr->nFilledLen         = 0;
                bufHdr->pAppPrivate        = appData;
                bufHdr->nOutputPortIndex   = PORT_INDEX_OUT;
                bufHdr->pBuffer            = NULL;
                bufHdr++;
                m_pOutput_pmem[i].fd = -1;
#ifdef USE_ION
                m_pOutput_ion[i].ion_device_fd =-1;
                m_pOutput_ion[i].fd_ion_data.fd=-1;
                m_pOutput_ion[i].ion_alloc_data.handle = 0;
#endif
            }
        } else {
            DEBUG_PRINT_ERROR("ERROR: Output buf mem alloc failed[0x%p]",m_out_mem_ptr);
            eRet =  OMX_ErrorInsufficientResources;
        }
    }

    for (i=0; i< m_sOutPortDef.nBufferCountActual; i++) {
        if (BITMASK_ABSENT(&m_out_bm_count,i)) {
            break;
        }
    }

    if (eRet == OMX_ErrorNone) {
        if (i < m_sOutPortDef.nBufferCountActual) {
            *bufferHdr = (m_out_mem_ptr + i );
            (*bufferHdr)->pBuffer = (OMX_U8 *)buffer;
            (*bufferHdr)->pAppPrivate = appData;

            if (!m_use_output_pmem) {
#ifdef USE_ION
#ifdef _MSM8974_
                align_size = (m_sOutPortDef.nBufferSize + (SZ_4K - 1)) & ~(SZ_4K - 1);
                m_pOutput_ion[i].ion_device_fd = alloc_map_ion_memory(align_size,
                        &m_pOutput_ion[i].ion_alloc_data,
                        &m_pOutput_ion[i].fd_ion_data,0);
#else
                m_pOutput_ion[i].ion_device_fd = alloc_map_ion_memory(
                        m_sOutPortDef.nBufferSize,
                        &m_pOutput_ion[i].ion_alloc_data,
                        &m_pOutput_ion[i].fd_ion_data,ION_FLAG_CACHED);
#endif
                if (m_pOutput_ion[i].ion_device_fd < 0) {
                    DEBUG_PRINT_ERROR("ERROR:ION device open() Failed");
                    return OMX_ErrorInsufficientResources;
                }
                m_pOutput_pmem[i].fd = m_pOutput_ion[i].fd_ion_data.fd;
#else
                m_pOutput_pmem[i].fd = open (MEM_DEVICE,O_RDWR);

                if (m_pOutput_pmem[i].fd == 0) {
                    m_pOutput_pmem[i].fd = open (MEM_DEVICE,O_RDWR);
                }

                if (m_pOutput_pmem[i].fd < 0) {
                    DEBUG_PRINT_ERROR("ERROR: /dev/pmem_adsp open() Failed");
                    return OMX_ErrorInsufficientResources;
                }
#endif
                m_pOutput_pmem[i].size = m_sOutPortDef.nBufferSize;
                m_pOutput_pmem[i].offset = 0;

                m_pOutput_pmem[i].buffer = (OMX_U8 *)SECURE_BUFPTR;
                if(!secure_session) {
#ifdef _MSM8974_
                    m_pOutput_pmem[i].buffer = (unsigned char *)mmap(NULL,
                        align_size,PROT_READ|PROT_WRITE,
                        MAP_SHARED,m_pOutput_pmem[i].fd,0);
#else
                    m_pOutput_pmem[i].buffer = (unsigned char *)mmap(NULL,
                        m_pOutput_pmem[i].size,PROT_READ|PROT_WRITE,
                        MAP_SHARED,m_pOutput_pmem[i].fd,0);
#endif
                if (m_pOutput_pmem[i].buffer == MAP_FAILED) {
                        DEBUG_PRINT_ERROR("ERROR: mmap() Failed");
                    close(m_pOutput_pmem[i].fd);
#ifdef USE_ION
                    free_ion_memory(&m_pOutput_ion[i]);
#endif
                    return OMX_ErrorInsufficientResources;
                }
                }
            } else {
                OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pParam = reinterpret_cast<OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO*>((*bufferHdr)->pAppPrivate);
                DEBUG_PRINT_LOW("Inside qcom_ext pParam: %p", pParam);

                if (pParam) {
                    DEBUG_PRINT_LOW("Inside qcom_ext with luma:(fd:%lu,offset:0x%x)", pParam->pmem_fd, (int)pParam->offset);
                    m_pOutput_pmem[i].fd = pParam->pmem_fd;
                    m_pOutput_pmem[i].offset = pParam->offset;
                    m_pOutput_pmem[i].size = m_sOutPortDef.nBufferSize;
                    m_pOutput_pmem[i].buffer = (unsigned char *)buffer;
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid AppData given for PMEM o/p UseBuffer case");
                    return OMX_ErrorBadParameter;
                }
                buf_addr = (unsigned char *)buffer;
            }

            DEBUG_PRINT_LOW("use_out:: bufhdr = %p, pBuffer = %p, m_pOutput_pmem[i].buffer = %p",
                    (*bufferHdr), (*bufferHdr)->pBuffer, m_pOutput_pmem[i].buffer);
            if (dev_use_buf(&m_pOutput_pmem[i],PORT_INDEX_OUT,i) != true) {
                DEBUG_PRINT_ERROR("ERROR: dev_use_buf Failed for o/p buf");
                return OMX_ErrorInsufficientResources;
            }

            BITMASK_SET(&m_out_bm_count,i);
        } else {
            DEBUG_PRINT_ERROR("ERROR: All o/p Buffers have been Used, invalid use_buf call for "
                    "index = %u", i);
            eRet = OMX_ErrorInsufficientResources;
        }
    }
    return eRet;
}


/* ======================================================================
   FUNCTION
   omx_video::UseBuffer

   DESCRIPTION
   OMX Use Buffer method implementation.

   PARAMETERS
   <TBD>.

   RETURN VALUE
   OMX Error None , if everything successful.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::use_buffer(
        OMX_IN OMX_HANDLETYPE            hComp,
        OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
        OMX_IN OMX_U32                   port,
        OMX_IN OMX_PTR                   appData,
        OMX_IN OMX_U32                   bytes,
        OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("ERROR: Use Buffer in Invalid State");
        return OMX_ErrorInvalidState;
    }
    if (port == PORT_INDEX_IN) {
        eRet = use_input_buffer(hComp,bufferHdr,port,appData,bytes,buffer);
    } else if (port == PORT_INDEX_OUT) {
        eRet = use_output_buffer(hComp,bufferHdr,port,appData,bytes,buffer);
    } else {
        DEBUG_PRINT_ERROR("ERROR: Invalid Port Index received %d",(int)port);
        eRet = OMX_ErrorBadPortIndex;
    }

    if (eRet == OMX_ErrorNone) {
        if (allocate_done()) {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)) {
                // Send the callback now
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_IDLE_PENDING);
                post_event(OMX_CommandStateSet,OMX_StateIdle,
                        OMX_COMPONENT_GENERATE_EVENT);
            }
        }
        if (port == PORT_INDEX_IN && m_sInPortDef.bPopulated) {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING)) {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
                post_event(OMX_CommandPortEnable,
                        PORT_INDEX_IN,
                        OMX_COMPONENT_GENERATE_EVENT);
            }

        } else if (port == PORT_INDEX_OUT && m_sOutPortDef.bPopulated) {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING)) {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                post_event(OMX_CommandPortEnable,
                        PORT_INDEX_OUT,
                        OMX_COMPONENT_GENERATE_EVENT);
                m_event_port_settings_sent = false;
            }
        }
    }
    return eRet;
}

OMX_ERRORTYPE omx_video::free_input_buffer(OMX_BUFFERHEADERTYPE *bufferHdr)
{
    unsigned int index = 0;
    OMX_U8 *temp_buff ;

    if (bufferHdr == NULL || m_inp_mem_ptr == NULL) {
        DEBUG_PRINT_ERROR("ERROR: free_input: Invalid bufferHdr[%p] or m_inp_mem_ptr[%p]",
                bufferHdr, m_inp_mem_ptr);
        return OMX_ErrorBadParameter;
    }

    index = bufferHdr - ((!mUseProxyColorFormat)?m_inp_mem_ptr:meta_buffer_hdr);
#ifdef _ANDROID_ICS_
    if (meta_mode_enable) {
        if (index < m_sInPortDef.nBufferCountActual) {
            memset(&meta_buffer_hdr[index], 0, sizeof(meta_buffer_hdr[index]));
            memset(&meta_buffers[index], 0, sizeof(meta_buffers[index]));
        }
        if (!mUseProxyColorFormat)
            return OMX_ErrorNone;
        else {
            c2d_conv.close();
            opaque_buffer_hdr[index] = NULL;
        }
    }
#endif
    if (index < m_sInPortDef.nBufferCountActual && !mUseProxyColorFormat &&
            dev_free_buf(&m_pInput_pmem[index],PORT_INDEX_IN) != true) {
        DEBUG_PRINT_LOW("ERROR: dev_free_buf() Failed for i/p buf");
    }

    if (index < m_sInPortDef.nBufferCountActual && m_pInput_pmem) {
        auto_lock l(m_lock);

        if (m_pInput_pmem[index].fd > 0 && input_use_buffer == false) {
            DEBUG_PRINT_LOW("FreeBuffer:: i/p AllocateBuffer case");
            if(!secure_session) {
            munmap (m_pInput_pmem[index].buffer,m_pInput_pmem[index].size);
            } else {
                free(m_pInput_pmem[index].buffer);
            }
            m_pInput_pmem[index].buffer = NULL;
            close (m_pInput_pmem[index].fd);
#ifdef USE_ION
            free_ion_memory(&m_pInput_ion[index]);
#endif
            m_pInput_pmem[index].fd = -1;
        } else if (m_pInput_pmem[index].fd > 0 && (input_use_buffer == true &&
                    m_use_input_pmem == OMX_FALSE)) {
            DEBUG_PRINT_LOW("FreeBuffer:: i/p Heap UseBuffer case");
            if (dev_free_buf(&m_pInput_pmem[index],PORT_INDEX_IN) != true) {
                DEBUG_PRINT_ERROR("ERROR: dev_free_buf() Failed for i/p buf");
            }
            if(!secure_session) {
            munmap (m_pInput_pmem[index].buffer,m_pInput_pmem[index].size);
            m_pInput_pmem[index].buffer = NULL;
            }
            close (m_pInput_pmem[index].fd);
#ifdef USE_ION
            free_ion_memory(&m_pInput_ion[index]);
#endif
            m_pInput_pmem[index].fd = -1;
        } else {
            DEBUG_PRINT_ERROR("FreeBuffer:: fd is invalid or i/p PMEM UseBuffer case");
        }
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_video::free_output_buffer(OMX_BUFFERHEADERTYPE *bufferHdr)
{
    unsigned int index = 0;
    OMX_U8 *temp_buff ;

    if (bufferHdr == NULL || m_out_mem_ptr == NULL) {
        DEBUG_PRINT_ERROR("ERROR: free_output: Invalid bufferHdr[%p] or m_out_mem_ptr[%p]",
                bufferHdr, m_out_mem_ptr);
        return OMX_ErrorBadParameter;
    }
    index = bufferHdr - m_out_mem_ptr;

    if (index < m_sOutPortDef.nBufferCountActual &&
            dev_free_buf(&m_pOutput_pmem[index],PORT_INDEX_OUT) != true) {
        DEBUG_PRINT_ERROR("ERROR: dev_free_buf Failed for o/p buf");
    }

    if (index < m_sOutPortDef.nBufferCountActual && m_pOutput_pmem) {
        if (m_pOutput_pmem[index].fd > 0 && output_use_buffer == false ) {
            DEBUG_PRINT_LOW("FreeBuffer:: o/p AllocateBuffer case");
            if(!secure_session) {
                munmap (m_pOutput_pmem[index].buffer,
                        m_pOutput_pmem[index].size);
            } else {
                char *data = (char*) m_pOutput_pmem[index].buffer;
                native_handle_t *handle = (native_handle_t*) data + 4;
                native_handle_delete(handle);
                free(m_pOutput_pmem[index].buffer);
            }
            close (m_pOutput_pmem[index].fd);
#ifdef USE_ION
            free_ion_memory(&m_pOutput_ion[index]);
#endif
            m_pOutput_pmem[index].fd = -1;
        } else if ( m_pOutput_pmem[index].fd > 0 && (output_use_buffer == true
                    && m_use_output_pmem == OMX_FALSE)) {
            DEBUG_PRINT_LOW("FreeBuffer:: o/p Heap UseBuffer case");
            if (dev_free_buf(&m_pOutput_pmem[index],PORT_INDEX_OUT) != true) {
                DEBUG_PRINT_ERROR("ERROR: dev_free_buf Failed for o/p buf");
            }
            if(!secure_session) {
                munmap (m_pOutput_pmem[index].buffer,
                        m_pOutput_pmem[index].size);
            }
            close (m_pOutput_pmem[index].fd);
#ifdef USE_ION
            free_ion_memory(&m_pOutput_ion[index]);
#endif
            m_pOutput_pmem[index].fd = -1;
        } else {
            DEBUG_PRINT_LOW("FreeBuffer:: fd is invalid or o/p PMEM UseBuffer case");
        }
    }
    return OMX_ErrorNone;
}
#ifdef _ANDROID_ICS_
OMX_ERRORTYPE omx_video::allocate_input_meta_buffer(
        OMX_HANDLETYPE       hComp,
        OMX_BUFFERHEADERTYPE **bufferHdr,
        OMX_PTR              appData,
        OMX_U32              bytes)
{
    unsigned index = 0;
    if (!bufferHdr || bytes != sizeof(encoder_media_buffer_type)) {
        DEBUG_PRINT_ERROR("wrong params allocate_input_meta_buffer Hdr %p len %u",
                bufferHdr, (unsigned int)bytes);
        return OMX_ErrorBadParameter;
    }

    if (!m_inp_mem_ptr && !mUseProxyColorFormat)
        m_inp_mem_ptr = meta_buffer_hdr;
    for (index = 0; ((index < m_sInPortDef.nBufferCountActual) &&
                meta_buffer_hdr[index].pBuffer); index++);
    if (index == m_sInPortDef.nBufferCountActual) {
        DEBUG_PRINT_ERROR("All buffers are allocated input_meta_buffer");
        return OMX_ErrorBadParameter;
    }
    if (mUseProxyColorFormat) {
        if (opaque_buffer_hdr[index]) {
            DEBUG_PRINT_ERROR("All buffers are allocated opaque_buffer_hdr");
            return OMX_ErrorBadParameter;
        }
        if (allocate_input_buffer(hComp,&opaque_buffer_hdr[index],
                    PORT_INDEX_IN,appData,m_sInPortDef.nBufferSize) != OMX_ErrorNone) {
            DEBUG_PRINT_ERROR("All buffers are allocated opaque_buffer_hdr");
            return OMX_ErrorBadParameter;
        }
    }
    BITMASK_SET(&m_inp_bm_count,index);
    *bufferHdr = &meta_buffer_hdr[index];
    memset(&meta_buffer_hdr[index], 0, sizeof(meta_buffer_hdr[index]));
    meta_buffer_hdr[index].nSize = sizeof(meta_buffer_hdr[index]);
    meta_buffer_hdr[index].nAllocLen = bytes;
    meta_buffer_hdr[index].nVersion.nVersion = OMX_SPEC_VERSION;
    meta_buffer_hdr[index].nInputPortIndex = PORT_INDEX_IN;
    meta_buffer_hdr[index].pBuffer = (OMX_U8*)&meta_buffers[index];
    meta_buffer_hdr[index].pAppPrivate = appData;
    if (mUseProxyColorFormat) {
        m_opq_pmem_q.insert_entry((unsigned long)opaque_buffer_hdr[index],0,0);
        DEBUG_PRINT_HIGH("opaque_buffer_hdr insert %p", opaque_buffer_hdr[index]);
    }
    return OMX_ErrorNone;
}
#endif
/* ======================================================================
   FUNCTION
   omx_venc::AllocateInputBuffer

   DESCRIPTION
   Helper function for allocate buffer in the input pin

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
OMX_ERRORTYPE  omx_video::allocate_input_buffer(
        OMX_IN OMX_HANDLETYPE            hComp,
        OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
        OMX_IN OMX_U32                   port,
        OMX_IN OMX_PTR                   appData,
        OMX_IN OMX_U32                   bytes)
{
    (void)hComp, (void)port;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    unsigned   i = 0;

    DEBUG_PRINT_HIGH("allocate_input_buffer()::");
    if (bytes != m_sInPortDef.nBufferSize) {
        DEBUG_PRINT_ERROR("ERROR: Buffer size mismatch error: bytes[%u] != nBufferSize[%u]",
                (unsigned int)bytes, (unsigned int)m_sInPortDef.nBufferSize);
        return OMX_ErrorBadParameter;
    }

    if (!m_inp_mem_ptr) {
        DEBUG_PRINT_HIGH("%s: size = %u, actual cnt %u", __FUNCTION__,
                (unsigned int)m_sInPortDef.nBufferSize, (unsigned int)m_sInPortDef.nBufferCountActual);
        m_inp_mem_ptr = (OMX_BUFFERHEADERTYPE*) \
                        calloc( (sizeof(OMX_BUFFERHEADERTYPE)), m_sInPortDef.nBufferCountActual);
        if (m_inp_mem_ptr == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_inp_mem_ptr");
            return OMX_ErrorInsufficientResources;
        }

        m_pInput_pmem = (struct pmem *) calloc(sizeof (struct pmem), m_sInPortDef.nBufferCountActual);

        if (m_pInput_pmem == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_pInput_pmem");
            return OMX_ErrorInsufficientResources;
        }
#ifdef USE_ION
        m_pInput_ion = (struct venc_ion *) calloc(sizeof (struct venc_ion), m_sInPortDef.nBufferCountActual);
        if (m_pInput_ion == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_pInput_ion");
            return OMX_ErrorInsufficientResources;
        }
#endif
        for (i=0; i< m_sInPortDef.nBufferCountActual; i++) {
            m_pInput_pmem[i].fd = -1;
#ifdef USE_ION
            m_pInput_ion[i].ion_device_fd =-1;
            m_pInput_ion[i].fd_ion_data.fd =-1;
            m_pInput_ion[i].ion_alloc_data.handle = 0;
#endif
        }
    }

    for (i=0; i< m_sInPortDef.nBufferCountActual; i++) {
        if (BITMASK_ABSENT(&m_inp_bm_count,i)) {
            break;
        }
    }
    if (i < m_sInPortDef.nBufferCountActual) {

        *bufferHdr = (m_inp_mem_ptr + i);
        (*bufferHdr)->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
        (*bufferHdr)->nVersion.nVersion = OMX_SPEC_VERSION;
        (*bufferHdr)->nAllocLen         = m_sInPortDef.nBufferSize;
        (*bufferHdr)->pAppPrivate       = appData;
        (*bufferHdr)->nInputPortIndex   = PORT_INDEX_IN;
        // make fd available to app layer, help with testing
        (*bufferHdr)->pInputPortPrivate = (OMX_PTR)&m_pInput_pmem[i];

#ifdef USE_ION
#ifdef _MSM8974_
        m_pInput_ion[i].ion_device_fd = alloc_map_ion_memory(m_sInPortDef.nBufferSize,
                &m_pInput_ion[i].ion_alloc_data,
                &m_pInput_ion[i].fd_ion_data,0);
#else
        m_pInput_ion[i].ion_device_fd = alloc_map_ion_memory(m_sInPortDef.nBufferSize,
                &m_pInput_ion[i].ion_alloc_data,
                &m_pInput_ion[i].fd_ion_data,ION_FLAG_CACHED);
#endif
        if (m_pInput_ion[i].ion_device_fd < 0) {
            DEBUG_PRINT_ERROR("ERROR:ION device open() Failed");
            return OMX_ErrorInsufficientResources;
        }

        m_pInput_pmem[i].fd = m_pInput_ion[i].fd_ion_data.fd;
#else
        m_pInput_pmem[i].fd = open (MEM_DEVICE,O_RDWR);

        if (m_pInput_pmem[i].fd == 0) {
            m_pInput_pmem[i].fd = open (MEM_DEVICE,O_RDWR);
        }

        if (m_pInput_pmem[i].fd < 0) {
            DEBUG_PRINT_ERROR("ERROR: /dev/pmem_adsp open() Failed");
            return OMX_ErrorInsufficientResources;
        }
#endif
        m_pInput_pmem[i].size = m_sInPortDef.nBufferSize;
        m_pInput_pmem[i].offset = 0;

        m_pInput_pmem[i].buffer = (OMX_U8 *)SECURE_BUFPTR;
        if(!secure_session) {
            m_pInput_pmem[i].buffer = (unsigned char *)mmap(NULL,
                m_pInput_pmem[i].size,PROT_READ|PROT_WRITE,
                MAP_SHARED,m_pInput_pmem[i].fd,0);
        if (m_pInput_pmem[i].buffer == MAP_FAILED) {
                DEBUG_PRINT_ERROR("ERROR: mmap FAILED= %d", errno);
            close(m_pInput_pmem[i].fd);
#ifdef USE_ION
            free_ion_memory(&m_pInput_ion[i]);
#endif
            return OMX_ErrorInsufficientResources;
        }
        } else {
            //This should only be used for passing reference to source type and
            //secure handle fd struct native_handle_t*
            m_pInput_pmem[i].buffer = malloc(sizeof(OMX_U32) + sizeof(native_handle_t*));
        }

        (*bufferHdr)->pBuffer           = (OMX_U8 *)m_pInput_pmem[i].buffer;
        DEBUG_PRINT_LOW("Virtual address in allocate buffer is %p", m_pInput_pmem[i].buffer);
        BITMASK_SET(&m_inp_bm_count,i);
        //here change the I/P param here from buf_adr to pmem
        if (!mUseProxyColorFormat && (dev_use_buf(&m_pInput_pmem[i],PORT_INDEX_IN,i) != true)) {
            DEBUG_PRINT_ERROR("ERROR: dev_use_buf FAILED for i/p buf");
            return OMX_ErrorInsufficientResources;
        }
    } else {
        DEBUG_PRINT_ERROR("ERROR: All i/p buffers are allocated, invalid allocate buf call"
                "for index [%d]", i);
        eRet = OMX_ErrorInsufficientResources;
    }

    return eRet;
}


/* ======================================================================
   FUNCTION
   omx_venc::AllocateOutputBuffer

   DESCRIPTION
   Helper fn for AllocateBuffer in the output pin

   PARAMETERS
   <TBD>.

   RETURN VALUE
   OMX Error None if everything went well.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::allocate_output_buffer(
        OMX_IN OMX_HANDLETYPE            hComp,
        OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
        OMX_IN OMX_U32                   port,
        OMX_IN OMX_PTR                   appData,
        OMX_IN OMX_U32                   bytes)
{
    (void)hComp, (void)port;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE       *bufHdr= NULL; // buffer header
    unsigned                         i= 0; // Temporary counter
#ifdef _MSM8974_
    int align_size;
#endif
    DEBUG_PRINT_HIGH("allocate_output_buffer()for %u bytes", (unsigned int)bytes);
    if (!m_out_mem_ptr) {
        int nBufHdrSize        = 0;
        DEBUG_PRINT_HIGH("%s: size = %u, actual cnt %u", __FUNCTION__,
                (unsigned int)m_sOutPortDef.nBufferSize, (unsigned int)m_sOutPortDef.nBufferCountActual);
        nBufHdrSize        = m_sOutPortDef.nBufferCountActual * sizeof(OMX_BUFFERHEADERTYPE);

        /*
         * Memory for output side involves the following:
         * 1. Array of Buffer Headers
         * 2. Bitmask array to hold the buffer allocation details
         * In order to minimize the memory management entire allocation
         * is done in one step.
         */
        m_out_mem_ptr = (OMX_BUFFERHEADERTYPE  *)calloc(nBufHdrSize,1);

#ifdef USE_ION
        m_pOutput_ion = (struct venc_ion *) calloc(sizeof (struct venc_ion), m_sOutPortDef.nBufferCountActual);
        if (m_pOutput_ion == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_pOutput_ion");
            return OMX_ErrorInsufficientResources;
        }
#endif
        m_pOutput_pmem = (struct pmem *) calloc(sizeof(struct pmem), m_sOutPortDef.nBufferCountActual);
        if (m_pOutput_pmem == NULL) {
            DEBUG_PRINT_ERROR("ERROR: calloc() Failed for m_pOutput_pmem");
            return OMX_ErrorInsufficientResources;
        }
        if (m_out_mem_ptr && m_pOutput_pmem) {
            bufHdr          =  m_out_mem_ptr;

            for (i=0; i < m_sOutPortDef.nBufferCountActual ; i++) {
                bufHdr->nSize              = sizeof(OMX_BUFFERHEADERTYPE);
                bufHdr->nVersion.nVersion  = OMX_SPEC_VERSION;
                // Set the values when we determine the right HxW param
                bufHdr->nAllocLen          = bytes;
                bufHdr->nFilledLen         = 0;
                bufHdr->pAppPrivate        = appData;
                bufHdr->nOutputPortIndex   = PORT_INDEX_OUT;
                // make fd available to app layer, help with testing
                bufHdr->pOutputPortPrivate = (OMX_PTR)&m_pOutput_pmem[i];
                bufHdr->pBuffer            = NULL;
                bufHdr++;
                m_pOutput_pmem[i].fd = -1;
#ifdef USE_ION
                m_pOutput_ion[i].ion_device_fd =-1;
                m_pOutput_ion[i].fd_ion_data.fd=-1;
                m_pOutput_ion[i].ion_alloc_data.handle = 0;
#endif
            }
        } else {
            DEBUG_PRINT_ERROR("ERROR: calloc() failed for m_out_mem_ptr/m_pOutput_pmem");
            eRet = OMX_ErrorInsufficientResources;
        }
    }

    DEBUG_PRINT_HIGH("actual cnt = %u", (unsigned int)m_sOutPortDef.nBufferCountActual);
    for (i=0; i< m_sOutPortDef.nBufferCountActual; i++) {
        if (BITMASK_ABSENT(&m_out_bm_count,i)) {
            DEBUG_PRINT_LOW("Found a Free Output Buffer %d",i);
            break;
        }
    }
    if (eRet == OMX_ErrorNone) {
        if (i < m_sOutPortDef.nBufferCountActual) {
#ifdef USE_ION
#ifdef _MSM8974_
            align_size = ((m_sOutPortDef.nBufferSize + 4095)/4096) * 4096;
            m_pOutput_ion[i].ion_device_fd = alloc_map_ion_memory(align_size,
                    &m_pOutput_ion[i].ion_alloc_data,
                    &m_pOutput_ion[i].fd_ion_data,0);
#else
            m_pOutput_ion[i].ion_device_fd = alloc_map_ion_memory(m_sOutPortDef.nBufferSize,
                    &m_pOutput_ion[i].ion_alloc_data,
                    &m_pOutput_ion[i].fd_ion_data,ION_FLAG_CACHED);
#endif
            if (m_pOutput_ion[i].ion_device_fd < 0) {
                DEBUG_PRINT_ERROR("ERROR:ION device open() Failed");
                return OMX_ErrorInsufficientResources;
            }

            m_pOutput_pmem[i].fd = m_pOutput_ion[i].fd_ion_data.fd;
#else
            m_pOutput_pmem[i].fd = open (MEM_DEVICE,O_RDWR);
            if (m_pOutput_pmem[i].fd == 0) {
                m_pOutput_pmem[i].fd = open (MEM_DEVICE,O_RDWR);
            }

            if (m_pOutput_pmem[i].fd < 0) {
                DEBUG_PRINT_ERROR("ERROR: /dev/pmem_adsp open() failed");
                return OMX_ErrorInsufficientResources;
            }
#endif
            m_pOutput_pmem[i].size = m_sOutPortDef.nBufferSize;
            m_pOutput_pmem[i].offset = 0;

            m_pOutput_pmem[i].buffer = (OMX_U8 *)SECURE_BUFPTR;
            if(!secure_session) {
#ifdef _MSM8974_
                m_pOutput_pmem[i].buffer = (unsigned char *)mmap(NULL,
                    align_size,PROT_READ|PROT_WRITE,
                    MAP_SHARED,m_pOutput_pmem[i].fd,0);
#else
                m_pOutput_pmem[i].buffer = (unsigned char *)mmap(NULL,
                    m_pOutput_pmem[i].size,PROT_READ|PROT_WRITE,
                    MAP_SHARED,m_pOutput_pmem[i].fd,0);
#endif
            if (m_pOutput_pmem[i].buffer == MAP_FAILED) {
                    DEBUG_PRINT_ERROR("ERROR: MMAP_FAILED in o/p alloc buffer");
                close (m_pOutput_pmem[i].fd);
#ifdef USE_ION
                free_ion_memory(&m_pOutput_ion[i]);
#endif
                return OMX_ErrorInsufficientResources;
            }
            }
            else {
                //This should only be used for passing reference to source type and
                //secure handle fd struct native_handle_t*
                m_pOutput_pmem[i].buffer = malloc(sizeof(OMX_U32) + sizeof(native_handle_t*));
                native_handle_t *handle = native_handle_create(1, 0);
                handle->data[0] = m_pOutput_pmem[i].fd;
                char *data = (char*) m_pOutput_pmem[i].buffer;
                OMX_U32 type = 1;
                memcpy(data, &type, 4);
                memcpy(data + 4, &handle, sizeof(native_handle_t*));
            }

            *bufferHdr = (m_out_mem_ptr + i );
            (*bufferHdr)->pBuffer = (OMX_U8 *)m_pOutput_pmem[i].buffer;
            (*bufferHdr)->pAppPrivate = appData;

            BITMASK_SET(&m_out_bm_count,i);

            if (dev_use_buf(&m_pOutput_pmem[i],PORT_INDEX_OUT,i) != true) {
                DEBUG_PRINT_ERROR("ERROR: dev_use_buf FAILED for o/p buf");
                return OMX_ErrorInsufficientResources;
            }
        } else {
            DEBUG_PRINT_ERROR("ERROR: All o/p buffers are allocated, invalid allocate buf call"
                    "for index [%d] actual: %u", i, (unsigned int)m_sOutPortDef.nBufferCountActual);
        }
    }

    return eRet;
}


// AllocateBuffer  -- API Call
/* ======================================================================
   FUNCTION
   omx_video::AllocateBuffer

   DESCRIPTION
   Returns zero if all the buffers released..

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
OMX_ERRORTYPE  omx_video::allocate_buffer(OMX_IN OMX_HANDLETYPE                hComp,
        OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
        OMX_IN OMX_U32                        port,
        OMX_IN OMX_PTR                     appData,
        OMX_IN OMX_U32                       bytes)
{

    OMX_ERRORTYPE eRet = OMX_ErrorNone; // OMX return type

    DEBUG_PRINT_LOW("Allocate buffer of size = %u on port %d", (unsigned int)bytes, (int)port);
    if (m_state == OMX_StateInvalid) {
        DEBUG_PRINT_ERROR("ERROR: Allocate Buf in Invalid State");
        return OMX_ErrorInvalidState;
    }

    // What if the client calls again.
    if (port == PORT_INDEX_IN) {
#ifdef _ANDROID_ICS_
        if (meta_mode_enable)
            eRet = allocate_input_meta_buffer(hComp,bufferHdr,appData,bytes);
        else
#endif
            eRet = allocate_input_buffer(hComp,bufferHdr,port,appData,bytes);
    } else if (port == PORT_INDEX_OUT) {
        eRet = allocate_output_buffer(hComp,bufferHdr,port,appData,bytes);
    } else {
        DEBUG_PRINT_ERROR("ERROR: Invalid Port Index received %d",(int)port);
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
        if (port == PORT_INDEX_IN && m_sInPortDef.bPopulated) {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING)) {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
                post_event(OMX_CommandPortEnable,
                        PORT_INDEX_IN,
                        OMX_COMPONENT_GENERATE_EVENT);
            }
        }
        if (port == PORT_INDEX_OUT && m_sOutPortDef.bPopulated) {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING)) {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                post_event(OMX_CommandPortEnable,
                        PORT_INDEX_OUT,
                        OMX_COMPONENT_GENERATE_EVENT);
                m_event_port_settings_sent = false;
            }
        }
    }
    DEBUG_PRINT_LOW("Allocate Buffer exit with ret Code %d",eRet);
    return eRet;
}


// Free Buffer - API call
/* ======================================================================
   FUNCTION
   omx_video::FreeBuffer

   DESCRIPTION

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
OMX_ERRORTYPE  omx_video::free_buffer(OMX_IN OMX_HANDLETYPE         hComp,
        OMX_IN OMX_U32                 port,
        OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    (void)hComp;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    unsigned int nPortIndex;

    DEBUG_PRINT_LOW("In for encoder free_buffer");

    if (m_state == OMX_StateIdle &&
            (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING))) {
        DEBUG_PRINT_LOW(" free buffer while Component in Loading pending");
    } else if ((m_sInPortDef.bEnabled == OMX_FALSE && port == PORT_INDEX_IN)||
            (m_sOutPortDef.bEnabled == OMX_FALSE && port == PORT_INDEX_OUT)) {
        DEBUG_PRINT_LOW("Free Buffer while port %u disabled", (unsigned int)port);
    } else if (m_state == OMX_StateExecuting || m_state == OMX_StatePause) {
        DEBUG_PRINT_ERROR("ERROR: Invalid state to free buffer,ports need to be disabled");
        post_event(OMX_EventError,
                OMX_ErrorPortUnpopulated,
                OMX_COMPONENT_GENERATE_EVENT);
        return eRet;
    } else {
        DEBUG_PRINT_ERROR("ERROR: Invalid state to free buffer,port lost Buffers");
        post_event(OMX_EventError,
                OMX_ErrorPortUnpopulated,
                OMX_COMPONENT_GENERATE_EVENT);
    }

    if (port == PORT_INDEX_IN) {
        // check if the buffer is valid
        nPortIndex = buffer - ((!mUseProxyColorFormat)?m_inp_mem_ptr:meta_buffer_hdr);

        DEBUG_PRINT_LOW("free_buffer on i/p port - Port idx %u, actual cnt %u",
                nPortIndex, (unsigned int)m_sInPortDef.nBufferCountActual);
        if (nPortIndex < m_sInPortDef.nBufferCountActual &&
                BITMASK_PRESENT(&m_inp_bm_count, nPortIndex)) {
            // Clear the bit associated with it.
            BITMASK_CLEAR(&m_inp_bm_count,nPortIndex);
            free_input_buffer (buffer);
            m_sInPortDef.bPopulated = OMX_FALSE;

            /*Free the Buffer Header*/
            if (release_input_done()
#ifdef _ANDROID_ICS_
                    && !meta_mode_enable
#endif
               ) {
                input_use_buffer = false;
                if (m_inp_mem_ptr) {
                    DEBUG_PRINT_LOW("Freeing m_inp_mem_ptr");
                    free (m_inp_mem_ptr);
                    m_inp_mem_ptr = NULL;
                }
                if (m_pInput_pmem) {
                    DEBUG_PRINT_LOW("Freeing m_pInput_pmem");
                    free(m_pInput_pmem);
                    m_pInput_pmem = NULL;
                }
#ifdef USE_ION
                if (m_pInput_ion) {
                    DEBUG_PRINT_LOW("Freeing m_pInput_ion");
                    free(m_pInput_ion);
                    m_pInput_ion = NULL;
                }
#endif
            }
        } else {
            DEBUG_PRINT_ERROR("ERROR: free_buffer ,Port Index Invalid");
            eRet = OMX_ErrorBadPortIndex;
        }

        if (BITMASK_PRESENT((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING)
                && release_input_done()) {
            DEBUG_PRINT_LOW("MOVING TO DISABLED STATE");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING);
            post_event(OMX_CommandPortDisable,
                    PORT_INDEX_IN,
                    OMX_COMPONENT_GENERATE_EVENT);
        }
    } else if (port == PORT_INDEX_OUT) {
        // check if the buffer is valid
        nPortIndex = buffer - (OMX_BUFFERHEADERTYPE*)m_out_mem_ptr;

        DEBUG_PRINT_LOW("free_buffer on o/p port - Port idx %u, actual cnt %u",
                nPortIndex, (unsigned int)m_sOutPortDef.nBufferCountActual);
        if (nPortIndex < m_sOutPortDef.nBufferCountActual &&
                BITMASK_PRESENT(&m_out_bm_count, nPortIndex)) {
            // Clear the bit associated with it.
            BITMASK_CLEAR(&m_out_bm_count,nPortIndex);
            m_sOutPortDef.bPopulated = OMX_FALSE;
            free_output_buffer (buffer);

            if (release_output_done()) {
                output_use_buffer = false;
                if (m_out_mem_ptr) {
                    DEBUG_PRINT_LOW("Freeing m_out_mem_ptr");
                    free (m_out_mem_ptr);
                    m_out_mem_ptr = NULL;
                }
                if (m_pOutput_pmem) {
                    DEBUG_PRINT_LOW("Freeing m_pOutput_pmem");
                    free(m_pOutput_pmem);
                    m_pOutput_pmem = NULL;
                }
#ifdef USE_ION
                if (m_pOutput_ion) {
                    DEBUG_PRINT_LOW("Freeing m_pOutput_ion");
                    free(m_pOutput_ion);
                    m_pOutput_ion = NULL;
                }
#endif
            }
        } else {
            DEBUG_PRINT_ERROR("ERROR: free_buffer , Port Index Invalid");
            eRet = OMX_ErrorBadPortIndex;
        }
        if (BITMASK_PRESENT((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING)
                && release_output_done() ) {
            DEBUG_PRINT_LOW("FreeBuffer : If any Disable event pending,post it");

            DEBUG_PRINT_LOW("MOVING TO DISABLED STATE");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
            post_event(OMX_CommandPortDisable,
                    PORT_INDEX_OUT,
                    OMX_COMPONENT_GENERATE_EVENT);

        }
    } else {
        eRet = OMX_ErrorBadPortIndex;
    }
    if ((eRet == OMX_ErrorNone) &&
            (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING))) {
        if (release_done()) {
            if (dev_stop() != 0) {
                DEBUG_PRINT_ERROR("ERROR: dev_stop() FAILED");
                eRet = OMX_ErrorHardware;
            }
            // Send the callback now
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_LOADING_PENDING);
            post_event(OMX_CommandStateSet, OMX_StateLoaded,
                    OMX_COMPONENT_GENERATE_EVENT);
        } else {
            DEBUG_PRINT_HIGH("in free buffer, release not done, need to free more buffers input 0x%x output 0x%x",
                    m_out_bm_count, m_inp_bm_count);
        }
    }

    return eRet;
}


/* ======================================================================
   FUNCTION
   omx_video::EmptyThisBuffer

   DESCRIPTION
   This routine is used to push the encoded video frames to
   the video decoder.

   PARAMETERS
   None.

   RETURN VALUE
   OMX Error None if everything went successful.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::empty_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
        OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE ret1 = OMX_ErrorNone;
    unsigned int nBufferIndex ;

    DEBUG_PRINT_LOW("ETB: buffer = %p, buffer->pBuffer[%p]", buffer, buffer->pBuffer);
    if (m_state != OMX_StateExecuting &&
            m_state != OMX_StatePause &&
            m_state != OMX_StateIdle) {
        DEBUG_PRINT_ERROR("ERROR: Empty this buffer in Invalid State");
        return OMX_ErrorInvalidState;
    }

    if (buffer == NULL || (buffer->nSize != sizeof(OMX_BUFFERHEADERTYPE))) {
        DEBUG_PRINT_ERROR("ERROR: omx_video::etb--> buffer is null or buffer size is invalid");
        return OMX_ErrorBadParameter;
    }

    if (buffer->nVersion.nVersion != OMX_SPEC_VERSION) {
        DEBUG_PRINT_ERROR("ERROR: omx_video::etb--> OMX Version Invalid");
        return OMX_ErrorVersionMismatch;
    }

    if (buffer->nInputPortIndex != (OMX_U32)PORT_INDEX_IN) {
        DEBUG_PRINT_ERROR("ERROR: Bad port index to call empty_this_buffer");
        return OMX_ErrorBadPortIndex;
    }
    if (!m_sInPortDef.bEnabled) {
        DEBUG_PRINT_ERROR("ERROR: Cannot call empty_this_buffer while I/P port is disabled");
        return OMX_ErrorIncorrectStateOperation;
    }

    nBufferIndex = buffer - ((!mUseProxyColorFormat)?m_inp_mem_ptr:meta_buffer_hdr);

    if (nBufferIndex > m_sInPortDef.nBufferCountActual ) {
        DEBUG_PRINT_ERROR("ERROR: ETB: Invalid buffer index[%d]", nBufferIndex);
        return OMX_ErrorBadParameter;
    }

    m_etb_count++;
    DEBUG_PRINT_LOW("DBG: i/p nTimestamp = %u", (unsigned)buffer->nTimeStamp);
    post_event ((unsigned long)hComp,(unsigned long)buffer,m_input_msg_id);
    return OMX_ErrorNone;
}
/* ======================================================================
   FUNCTION
   omx_video::empty_this_buffer_proxy

   DESCRIPTION
   This routine is used to push the encoded video frames to
   the video decoder.

   PARAMETERS
   None.

   RETURN VALUE
   OMX Error None if everything went successful.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::empty_this_buffer_proxy(OMX_IN OMX_HANDLETYPE  hComp,
        OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    (void)hComp;
    OMX_U8 *pmem_data_buf = NULL;
    int push_cnt = 0;
    unsigned nBufIndex = 0;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    encoder_media_buffer_type *media_buffer = NULL;

#ifdef _MSM8974_
    int fd = 0;
#endif
    DEBUG_PRINT_LOW("ETBProxy: buffer->pBuffer[%p]", buffer->pBuffer);
    if (buffer == NULL) {
        DEBUG_PRINT_ERROR("ERROR: ETBProxy: Invalid buffer[%p]", buffer);
        return OMX_ErrorBadParameter;
    }

    // Buffer sanity checks
    if (meta_mode_enable && !mUsesColorConversion) {
        //For color-conversion case, we have an internal buffer and not a meta buffer
        bool met_error = false;
        nBufIndex = buffer - meta_buffer_hdr;
        if (nBufIndex >= m_sInPortDef.nBufferCountActual) {
            DEBUG_PRINT_ERROR("ERROR: ETBProxy: Invalid meta-bufIndex = %u", nBufIndex);
            return OMX_ErrorBadParameter;
        }
        media_buffer = (encoder_media_buffer_type *)meta_buffer_hdr[nBufIndex].pBuffer;
        if (media_buffer) {
            if (media_buffer->buffer_type != kMetadataBufferTypeCameraSource &&
                    media_buffer->buffer_type != kMetadataBufferTypeGrallocSource) {
                met_error = true;
            } else {
                if (media_buffer->buffer_type == kMetadataBufferTypeCameraSource) {
                    if (media_buffer->meta_handle == NULL)
                        met_error = true;
                    else if ((media_buffer->meta_handle->numFds != 1 &&
                                media_buffer->meta_handle->numInts != 2))
                        met_error = true;
                }
            }
        } else
            met_error = true;
        if (met_error) {
            DEBUG_PRINT_ERROR("ERROR: Unkown source/metahandle in ETB call");
            post_event ((unsigned long)buffer,0,OMX_COMPONENT_GENERATE_EBD);
            return OMX_ErrorBadParameter;
        }
    } else {
        nBufIndex = buffer - ((OMX_BUFFERHEADERTYPE *)m_inp_mem_ptr);
        if (nBufIndex >= m_sInPortDef.nBufferCountActual) {
            DEBUG_PRINT_ERROR("ERROR: ETBProxy: Invalid bufIndex = %u", nBufIndex);
            return OMX_ErrorBadParameter;
        }
    }

    pending_input_buffers++;
    if (input_flush_progress == true) {
        post_event ((unsigned long)buffer,0,
                OMX_COMPONENT_GENERATE_EBD);
        DEBUG_PRINT_ERROR("ERROR: ETBProxy: Input flush in progress");
        return OMX_ErrorNone;
    }
#ifdef _MSM8974_
    if (!meta_mode_enable) {
        fd = m_pInput_pmem[nBufIndex].fd;
    }
#endif
#ifdef _ANDROID_ICS_
    if (meta_mode_enable && !mUseProxyColorFormat) {
        // Camera or Gralloc-source meta-buffers queued with pre-announced color-format
        struct pmem Input_pmem_info;
        if (media_buffer->buffer_type == kMetadataBufferTypeCameraSource) {
            Input_pmem_info.buffer = media_buffer;
            Input_pmem_info.fd = media_buffer->meta_handle->data[0];
#ifdef _MSM8974_
            fd = Input_pmem_info.fd;
#endif
            Input_pmem_info.offset = media_buffer->meta_handle->data[1];
            Input_pmem_info.size = media_buffer->meta_handle->data[2];
            DEBUG_PRINT_LOW("ETB (meta-Camera) fd = %d, offset = %d, size = %d",
                    Input_pmem_info.fd, Input_pmem_info.offset,
                    Input_pmem_info.size);
        } else {
            private_handle_t *handle = (private_handle_t *)media_buffer->meta_handle;
            Input_pmem_info.buffer = media_buffer;
            Input_pmem_info.fd = handle->fd;
#ifdef _MSM8974_
            fd = Input_pmem_info.fd;
#endif
            Input_pmem_info.offset = 0;
            Input_pmem_info.size = handle->size;
            DEBUG_PRINT_LOW("ETB (meta-gralloc) fd = %d, offset = %d, size = %d",
                    Input_pmem_info.fd, Input_pmem_info.offset,
                    Input_pmem_info.size);
        }
        if (dev_use_buf(&Input_pmem_info,PORT_INDEX_IN,0) != true) {
            DEBUG_PRINT_ERROR("ERROR: in dev_use_buf");
            post_event ((unsigned long)buffer,0,OMX_COMPONENT_GENERATE_EBD);
            return OMX_ErrorBadParameter;
        }
    } else if (meta_mode_enable && !mUsesColorConversion) {
        // Graphic-source meta-buffers queued with opaque color-format
        if (media_buffer->buffer_type == kMetadataBufferTypeGrallocSource) {
            private_handle_t *handle = (private_handle_t *)media_buffer->meta_handle;
            fd = handle->fd;
            DEBUG_PRINT_LOW("ETB (opaque-gralloc) fd = %d, size = %d",
                    fd, handle->size);
        } else {
            DEBUG_PRINT_ERROR("ERROR: Invalid bufferType for buffer with Opaque"
                    " color format");
            post_event ((unsigned long)buffer,0,OMX_COMPONENT_GENERATE_EBD);
            return OMX_ErrorBadParameter;
        }
    } else if (input_use_buffer && !m_use_input_pmem)
#else
    if (input_use_buffer && !m_use_input_pmem)
#endif
    {
        DEBUG_PRINT_LOW("Heap UseBuffer case, so memcpy the data");

        auto_lock l(m_lock);
        pmem_data_buf = (OMX_U8 *)m_pInput_pmem[nBufIndex].buffer;
        if (pmem_data_buf) {
            memcpy (pmem_data_buf, (buffer->pBuffer + buffer->nOffset),
                    buffer->nFilledLen);
        }
        DEBUG_PRINT_LOW("memcpy() done in ETBProxy for i/p Heap UseBuf");
    } else if (mUseProxyColorFormat) {
        // Gralloc-source buffers with color-conversion
        fd = m_pInput_pmem[nBufIndex].fd;
        DEBUG_PRINT_LOW("ETB (color-converted) fd = %d, size = %u",
                fd, (unsigned int)buffer->nFilledLen);
    } else if (m_sInPortDef.format.video.eColorFormat ==
                    OMX_COLOR_FormatYUV420SemiPlanar) {
            //For the case where YUV420SP buffers are qeueued to component
            //by sources other than camera (Apps via MediaCodec), conversion
            //to vendor flavoured NV12 color format is required.
            if (!dev_color_align(buffer, m_sInPortDef.format.video.nFrameWidth,
                                    m_sInPortDef.format.video.nFrameHeight)) {
                    DEBUG_PRINT_ERROR("Failed to adjust buffer color");
                    post_event((unsigned long)buffer, 0, OMX_COMPONENT_GENERATE_EBD);
                    return OMX_ErrorUndefined;
            }
    }
#ifdef _MSM8974_
    if (dev_empty_buf(buffer, pmem_data_buf,nBufIndex,fd) != true)
#else
    if (dev_empty_buf(buffer, pmem_data_buf,0,0) != true)
#endif
    {
        DEBUG_PRINT_ERROR("ERROR: ETBProxy: dev_empty_buf failed");
#ifdef _ANDROID_ICS_
        omx_release_meta_buffer(buffer);
#endif
        post_event ((unsigned long)buffer,0,OMX_COMPONENT_GENERATE_EBD);
        /*Generate an async error and move to invalid state*/
        pending_input_buffers--;
        return OMX_ErrorBadParameter;
    }
    return ret;
}

/* ======================================================================
   FUNCTION
   omx_video::FillThisBuffer

   DESCRIPTION
   IL client uses this method to release the frame buffer
   after displaying them.

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
OMX_ERRORTYPE  omx_video::fill_this_buffer(OMX_IN OMX_HANDLETYPE  hComp,
        OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    DEBUG_PRINT_LOW("FTB: buffer->pBuffer[%p]", buffer->pBuffer);
    if (m_state != OMX_StateExecuting &&
            m_state != OMX_StatePause &&
            m_state != OMX_StateIdle) {
        DEBUG_PRINT_ERROR("ERROR: FTB in Invalid State");
        return OMX_ErrorInvalidState;
    }

    if (buffer == NULL ||(buffer->nSize != sizeof(OMX_BUFFERHEADERTYPE))) {
        DEBUG_PRINT_ERROR("ERROR: omx_video::ftb-->Invalid buffer or size");
        return OMX_ErrorBadParameter;
    }

    if (buffer->nVersion.nVersion != OMX_SPEC_VERSION) {
        DEBUG_PRINT_ERROR("ERROR: omx_video::ftb-->OMX Version Invalid");
        return OMX_ErrorVersionMismatch;
    }

    if (buffer->nOutputPortIndex != (OMX_U32)PORT_INDEX_OUT) {
        DEBUG_PRINT_ERROR("ERROR: omx_video::ftb-->Bad port index");
        return OMX_ErrorBadPortIndex;
    }

    if (!m_sOutPortDef.bEnabled) {
        DEBUG_PRINT_ERROR("ERROR: omx_video::ftb-->port is disabled");
        return OMX_ErrorIncorrectStateOperation;
    }

    post_event((unsigned long) hComp, (unsigned long)buffer,OMX_COMPONENT_GENERATE_FTB);
    return OMX_ErrorNone;
}

/* ======================================================================
   FUNCTION
   omx_video::fill_this_buffer_proxy

   DESCRIPTION
   IL client uses this method to release the frame buffer
   after displaying them.

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
OMX_ERRORTYPE  omx_video::fill_this_buffer_proxy(
        OMX_IN OMX_HANDLETYPE        hComp,
        OMX_IN OMX_BUFFERHEADERTYPE* bufferAdd)
{
    (void)hComp;
    OMX_U8 *pmem_data_buf = NULL;
    OMX_ERRORTYPE nRet = OMX_ErrorNone;

    DEBUG_PRINT_LOW("FTBProxy: bufferAdd->pBuffer[%p]", bufferAdd->pBuffer);

    if (bufferAdd == NULL || ((bufferAdd - m_out_mem_ptr) >= (int)m_sOutPortDef.nBufferCountActual) ) {
        DEBUG_PRINT_ERROR("ERROR: FTBProxy: Invalid i/p params");
        return OMX_ErrorBadParameter;
    }

    pending_output_buffers++;
    /*Return back the output buffer to client*/
    if ( m_sOutPortDef.bEnabled != OMX_TRUE || output_flush_progress == true) {
        DEBUG_PRINT_LOW("o/p port is Disabled or Flush in Progress");
        post_event ((unsigned long)bufferAdd,0,
                OMX_COMPONENT_GENERATE_FBD);
        return OMX_ErrorNone;
    }

    if (output_use_buffer && !m_use_output_pmem) {
        DEBUG_PRINT_LOW("Heap UseBuffer case");
        pmem_data_buf = (OMX_U8 *)m_pOutput_pmem[bufferAdd - m_out_mem_ptr].buffer;
    }

    if (dev_fill_buf(bufferAdd, pmem_data_buf,(bufferAdd - m_out_mem_ptr),m_pOutput_pmem[bufferAdd - m_out_mem_ptr].fd) != true) {
        DEBUG_PRINT_ERROR("ERROR: dev_fill_buf() Failed");
        post_event ((unsigned long)bufferAdd,0,OMX_COMPONENT_GENERATE_FBD);
        pending_output_buffers--;
        return OMX_ErrorBadParameter;
    }

    return OMX_ErrorNone;
}

/* ======================================================================
   FUNCTION
   omx_video::SetCallbacks

   DESCRIPTION
   Set the callbacks.

   PARAMETERS
   None.

   RETURN VALUE
   OMX Error None if everything successful.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
        OMX_IN OMX_CALLBACKTYPE* callbacks,
        OMX_IN OMX_PTR             appData)
{
    (void)hComp;
    m_pCallbacks       = *callbacks;
    DEBUG_PRINT_LOW("Callbacks Set %p %p %p",m_pCallbacks.EmptyBufferDone,\
            m_pCallbacks.EventHandler,m_pCallbacks.FillBufferDone);
    m_app_data =    appData;
    return OMX_ErrorNotImplemented;
}


/* ======================================================================
   FUNCTION
   omx_venc::UseEGLImage

   DESCRIPTION
   OMX Use EGL Image method implementation <TBD>.

   PARAMETERS
   <TBD>.

   RETURN VALUE
   Not Implemented error.

   ========================================================================== */
OMX_ERRORTYPE  omx_video::use_EGL_image(OMX_IN OMX_HANDLETYPE   hComp,
        OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
        OMX_IN OMX_U32                        port,
        OMX_IN OMX_PTR                     appData,
        OMX_IN void*                      eglImage)
{
    (void)hComp, (void)bufferHdr, (void)port, (void)appData, (void)eglImage;
    DEBUG_PRINT_ERROR("ERROR: use_EGL_image:  Not Implemented");
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
   FUNCTION
   omx_venc::ComponentRoleEnum

   DESCRIPTION
   OMX Component Role Enum method implementation.

   PARAMETERS
   <TBD>.

   RETURN VALUE
   OMX Error None if everything is successful.
   ========================================================================== */
OMX_ERRORTYPE  omx_video::component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
        OMX_OUT OMX_U8*        role,
        OMX_IN OMX_U32        index)
{
    (void)hComp;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if (!strncmp((char*)m_nkind, "OMX.qcom.video.decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            eRet = OMX_ErrorNoMore;
        }
    } else if (!strncmp((char*)m_nkind, "OMX.qcom.video.decoder.h263",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            DEBUG_PRINT_ERROR("ERROR: No more roles");
            eRet = OMX_ErrorNoMore;
        }
    } else if (!strncmp((char*)m_nkind, "OMX.qcom.video.decoder.avc",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            DEBUG_PRINT_ERROR("ERROR: No more roles");
            eRet = OMX_ErrorNoMore;
        }
    } else if (!strncmp((char*)m_nkind, "OMX.qcom.video.decoder.vc1",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            DEBUG_PRINT_ERROR("ERROR: No more roles");
            eRet = OMX_ErrorNoMore;
        }
    }
    if (!strncmp((char*)m_nkind, "OMX.qcom.video.encoder.mpeg4",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_encoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            eRet = OMX_ErrorNoMore;
        }
    } else if (!strncmp((char*)m_nkind, "OMX.qcom.video.encoder.h263",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_encoder.h263",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            DEBUG_PRINT_ERROR("ERROR: No more roles");
            eRet = OMX_ErrorNoMore;
        }
    } else if (!strncmp((char*)m_nkind, "OMX.qcom.video.encoder.avc",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_encoder.avc",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            DEBUG_PRINT_ERROR("ERROR: No more roles");
            eRet = OMX_ErrorNoMore;
        }
    }
#ifdef _MSM8974_
    else if (!strncmp((char*)m_nkind, "OMX.qcom.video.encoder.vp8",OMX_MAX_STRINGNAME_SIZE)) {
        if ((0 == index) && role) {
            strlcpy((char *)role, "video_encoder.vp8",OMX_MAX_STRINGNAME_SIZE);
            DEBUG_PRINT_LOW("component_role_enum: role %s",role);
        } else {
            DEBUG_PRINT_ERROR("ERROR: No more roles");
            eRet = OMX_ErrorNoMore;
        }
    }
#endif
    else {
        DEBUG_PRINT_ERROR("ERROR: Querying Role on Unknown Component");
        eRet = OMX_ErrorInvalidComponentName;
    }
    return eRet;
}




/* ======================================================================
   FUNCTION
   omx_venc::AllocateDone

   DESCRIPTION
   Checks if entire buffer pool is allocated by IL Client or not.
   Need this to move to IDLE state.

   PARAMETERS
   None.

   RETURN VALUE
   true/false.

   ========================================================================== */
bool omx_video::allocate_done(void)
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
   omx_venc::AllocateInputDone

   DESCRIPTION
   Checks if I/P buffer pool is allocated by IL Client or not.

   PARAMETERS
   None.

   RETURN VALUE
   true/false.

   ========================================================================== */
bool omx_video::allocate_input_done(void)
{
    bool bRet = false;
    unsigned i=0;

    if (m_inp_mem_ptr == NULL) {
        return bRet;
    }
    if (m_inp_mem_ptr ) {
        for (; i<m_sInPortDef.nBufferCountActual; i++) {
            if (BITMASK_ABSENT(&m_inp_bm_count,i)) {
                break;
            }
        }
    }
    if (i==m_sInPortDef.nBufferCountActual) {
        bRet = true;
    }
    if (i==m_sInPortDef.nBufferCountActual && m_sInPortDef.bEnabled) {
        m_sInPortDef.bPopulated = OMX_TRUE;
    }
    return bRet;
}
/* ======================================================================
   FUNCTION
   omx_venc::AllocateOutputDone

   DESCRIPTION
   Checks if entire O/P buffer pool is allocated by IL Client or not.

   PARAMETERS
   None.

   RETURN VALUE
   true/false.

   ========================================================================== */
bool omx_video::allocate_output_done(void)
{
    bool bRet = false;
    unsigned j=0;

    if (m_out_mem_ptr == NULL) {
        return bRet;
    }

    if (m_out_mem_ptr ) {
        for (; j<m_sOutPortDef.nBufferCountActual; j++) {
            if (BITMASK_ABSENT(&m_out_bm_count,j)) {
                break;
            }
        }
    }

    if (j==m_sOutPortDef.nBufferCountActual) {
        bRet = true;
    }

    if (j==m_sOutPortDef.nBufferCountActual && m_sOutPortDef.bEnabled) {
        m_sOutPortDef.bPopulated = OMX_TRUE;
    }
    return bRet;
}

/* ======================================================================
   FUNCTION
   omx_venc::ReleaseDone

   DESCRIPTION
   Checks if IL client has released all the buffers.

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
bool omx_video::release_done(void)
{
    bool bRet = false;
    DEBUG_PRINT_LOW("Inside release_done()");
    if (release_input_done()) {
        if (release_output_done()) {
            bRet = true;
        }
    }
    return bRet;
}


/* ======================================================================
   FUNCTION
   omx_venc::ReleaseOutputDone

   DESCRIPTION
   Checks if IL client has released all the buffers.

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
bool omx_video::release_output_done(void)
{
    bool bRet = false;
    unsigned i=0,j=0;

    DEBUG_PRINT_LOW("Inside release_output_done()");
    if (m_out_mem_ptr) {
        for (; j<m_sOutPortDef.nBufferCountActual; j++) {
            if (BITMASK_PRESENT(&m_out_bm_count,j)) {
                break;
            }
        }
        if (j==m_sOutPortDef.nBufferCountActual) {
            bRet = true;
        }
    } else {
        bRet = true;
    }
    return bRet;
}
/* ======================================================================
   FUNCTION
   omx_venc::ReleaseInputDone

   DESCRIPTION
   Checks if IL client has released all the buffers.

   PARAMETERS
   None.

   RETURN VALUE
   true/false

   ========================================================================== */
bool omx_video::release_input_done(void)
{
    bool bRet = false;
    unsigned i=0,j=0;

    DEBUG_PRINT_LOW("Inside release_input_done()");
    if (m_inp_mem_ptr) {
        for (; j<m_sInPortDef.nBufferCountActual; j++) {
            if ( BITMASK_PRESENT(&m_inp_bm_count,j)) {
                break;
            }
        }
        if (j==m_sInPortDef.nBufferCountActual) {
            bRet = true;
        }
    } else {
        bRet = true;
    }
    return bRet;
}

OMX_ERRORTYPE omx_video::fill_buffer_done(OMX_HANDLETYPE hComp,
        OMX_BUFFERHEADERTYPE * buffer)
{
#ifdef _MSM8974_
    int index = buffer - m_out_mem_ptr;
#endif
    DEBUG_PRINT_LOW("fill_buffer_done: buffer->pBuffer[%p], flags=0x%x size = %u",
            buffer->pBuffer, (unsigned)buffer->nFlags, (unsigned int)buffer->nFilledLen);
    if (buffer == NULL || ((buffer - m_out_mem_ptr) > (int)m_sOutPortDef.nBufferCountActual)) {
        return OMX_ErrorBadParameter;
    }

    pending_output_buffers--;

    if(!secure_session) {
    extra_data_handle.create_extra_data(buffer);
#ifndef _MSM8974_
    if (buffer->nFlags & OMX_BUFFERFLAG_EXTRADATA) {
        DEBUG_PRINT_LOW("parsing extradata");
        extra_data_handle.parse_extra_data(buffer);
    }
#endif
    }

    /* For use buffer we need to copy the data */
    if (m_pCallbacks.FillBufferDone) {
        if (buffer->nFilledLen > 0) {
            m_fbd_count++;

            if (dev_get_output_log_flag()) {
                dev_output_log_buffers((const char*)buffer->pBuffer, buffer->nFilledLen);
            }
        }
#ifdef _MSM8974_
        if (buffer->nFlags & OMX_BUFFERFLAG_EXTRADATA) {
            if (!dev_handle_extradata((void *)buffer, index))
                DEBUG_PRINT_ERROR("Failed to parse extradata");

            dev_extradata_log_buffers((char *)(((unsigned long)buffer->pBuffer + buffer->nOffset +
                        buffer->nFilledLen + 3) & (~3)));
        }
#endif
        m_pCallbacks.FillBufferDone (hComp,m_app_data,buffer);
    } else {
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_video::empty_buffer_done(OMX_HANDLETYPE         hComp,
        OMX_BUFFERHEADERTYPE* buffer)
{
    int buffer_index  = -1;
    int buffer_index_meta = -1;

    buffer_index = (buffer - m_inp_mem_ptr);
    buffer_index_meta = (buffer - meta_buffer_hdr);
    DEBUG_PRINT_LOW("empty_buffer_done: buffer[%p]", buffer);
    if (buffer == NULL ||
            ((buffer_index > (int)m_sInPortDef.nBufferCountActual) &&
             (buffer_index_meta > (int)m_sInPortDef.nBufferCountActual))) {
        DEBUG_PRINT_ERROR("ERROR in empty_buffer_done due to index buffer");
        return OMX_ErrorBadParameter;
    }

    pending_input_buffers--;

    if (mUseProxyColorFormat && ((OMX_U32)buffer_index < m_sInPortDef.nBufferCountActual)) {
        if (!pdest_frame  && !input_flush_progress) {
            pdest_frame = buffer;
            DEBUG_PRINT_LOW("empty_buffer_done pdest_frame address is %p",pdest_frame);
            return push_input_buffer(hComp);
        }
        //check if empty-EOS-buffer is being returned, treat this same as the
        //color-conversion case as we queued a color-conversion buffer to encoder
        bool handleEmptyEosBuffer = (mEmptyEosBuffer == buffer);
        if (mUsesColorConversion || handleEmptyEosBuffer) {
            if (handleEmptyEosBuffer) {
                mEmptyEosBuffer = NULL;
            }
            // return color-conversion buffer back to the pool
            DEBUG_PRINT_LOW("empty_buffer_done insert address is %p",buffer);
            if (!m_opq_pmem_q.insert_entry((unsigned long)buffer, 0, 0)) {
                DEBUG_PRINT_ERROR("empty_buffer_done: pmem queue is full");
                return OMX_ErrorBadParameter;
            }
        } else {
            // We are not dealing with color-conversion, Buffer being returned
            // here is client's buffer, return it back to client
            OMX_BUFFERHEADERTYPE* il_buffer = &meta_buffer_hdr[buffer_index];
            if (m_pCallbacks.EmptyBufferDone && il_buffer) {
                m_pCallbacks.EmptyBufferDone(hComp, m_app_data, il_buffer);
                DEBUG_PRINT_LOW("empty_buffer_done: Returning client buf %p",il_buffer);
            }
        }
    } else if (m_pCallbacks.EmptyBufferDone) {
        m_pCallbacks.EmptyBufferDone(hComp ,m_app_data, buffer);
    }
    return OMX_ErrorNone;
}

void omx_video::complete_pending_buffer_done_cbs()
{
    unsigned long p1;
    unsigned long p2;
    unsigned long ident;
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

#ifdef MAX_RES_720P
OMX_ERRORTYPE omx_video::get_supported_profile_level(OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevelType)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if (!profileLevelType)
        return OMX_ErrorBadParameter;

    if (profileLevelType->nPortIndex == 1) {
        if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingAVC) {
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
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d",
                        (int)profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingH263) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_H263ProfileBaseline;
                profileLevelType->eLevel   = OMX_VIDEO_H263Level70;
            } else {
                DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d", (int)profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
                profileLevelType->eLevel   = OMX_VIDEO_MPEG4Level5;
            } else if (profileLevelType->nProfileIndex == 1) {
                profileLevelType->eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
                profileLevelType->eLevel   = OMX_VIDEO_MPEG4Level5;
            } else {
                DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d", (int)profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        }
    } else {
        DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported should be queried on Input port only %d", (int)profileLevelType->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
    }
    DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported for Input port returned Profile:%d, Level:%d",
            (int)profileLevelType->eProfile, (int)profileLevelType->eLevel);
    return eRet;
}
#endif

#ifdef MAX_RES_1080P
OMX_ERRORTYPE omx_video::get_supported_profile_level(OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevelType)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if (!profileLevelType)
        return OMX_ErrorBadParameter;

    if (profileLevelType->nPortIndex == 1) {
        if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingAVC) {
#if defined _MSM8974_ && !defined _MSM8226_
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileBaseline;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel52;
            } else if (profileLevelType->nProfileIndex == 1) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileMain;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel52;
            } else if (profileLevelType->nProfileIndex == 2) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileHigh;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel52;
            } else if (profileLevelType->nProfileIndex == 3) {
                profileLevelType->eProfile = QOMX_VIDEO_AVCProfileConstrainedBaseline;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel52;
            } else {
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %u",
                        (unsigned int)profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
#else
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileBaseline;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel4;

            } else if (profileLevelType->nProfileIndex == 1) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileMain;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel4;
            } else if (profileLevelType->nProfileIndex == 2) {
                profileLevelType->eProfile = OMX_VIDEO_AVCProfileHigh;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel4;
#ifdef _MSM8226_
            } else if (profileLevelType->nProfileIndex == 3) {
                profileLevelType->eProfile = QOMX_VIDEO_AVCProfileConstrainedBaseline;
                profileLevelType->eLevel   = OMX_VIDEO_AVCLevel4;
#endif
            } else {
                DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %d",
                        (int)profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
#endif
        } else if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingH263) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_H263ProfileBaseline;
                profileLevelType->eLevel   = OMX_VIDEO_H263Level70;
            } else {
                DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %u", (unsigned int)profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMPEG4) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
                profileLevelType->eLevel   = OMX_VIDEO_MPEG4Level5;
            } else if (profileLevelType->nProfileIndex == 1) {
                profileLevelType->eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
                profileLevelType->eLevel   = OMX_VIDEO_MPEG4Level5;
            } else {
                DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %u", (unsigned int)profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else if (m_sOutPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingVP8) {
            if (profileLevelType->nProfileIndex == 0) {
                profileLevelType->eProfile = OMX_VIDEO_VP8ProfileMain;
                profileLevelType->eLevel   = OMX_VIDEO_VP8Level_Version0;
            } else if (profileLevelType->nProfileIndex == 1) {
                profileLevelType->eProfile = OMX_VIDEO_VP8ProfileMain;
                profileLevelType->eLevel   = OMX_VIDEO_VP8Level_Version1;
            } else {
                DEBUG_PRINT_LOW("VP8: get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported nProfileIndex ret NoMore %u",
                (unsigned int)profileLevelType->nProfileIndex);
                eRet = OMX_ErrorNoMore;
            }
        } else {
            DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported ret NoMore");
            eRet = OMX_ErrorNoMore;
        }
    } else {
        DEBUG_PRINT_ERROR("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported should be queried on Input port only %u", (unsigned int)profileLevelType->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
    }
    DEBUG_PRINT_LOW("get_parameter: OMX_IndexParamVideoProfileLevelQuerySupported for Input port returned Profile:%u, Level:%u",
            (unsigned int)profileLevelType->eProfile, (unsigned int)profileLevelType->eLevel);
    return eRet;
}
#endif

#ifdef USE_ION
int omx_video::alloc_map_ion_memory(int size,
        struct ion_allocation_data *alloc_data,
        struct ion_fd_data *fd_data,int flag)
{
    struct venc_ion buf_ion_info;
    int ion_device_fd =-1,rc=0,ion_dev_flags = 0;
    if (size <=0 || !alloc_data || !fd_data) {
        DEBUG_PRINT_ERROR("Invalid input to alloc_map_ion_memory");
        return -EINVAL;
    }

    ion_dev_flags = O_RDONLY;
    ion_device_fd = open (MEM_DEVICE,ion_dev_flags);
    if (ion_device_fd < 0) {
        DEBUG_PRINT_ERROR("ERROR: ION Device open() Failed");
        return ion_device_fd;
    }

    if(secure_session) {
        alloc_data->len = (size + (SZ_1M - 1)) & ~(SZ_1M - 1);
        alloc_data->align = SZ_1M;
        alloc_data->flags = ION_SECURE;
        alloc_data->ION_HEAP_MASK = ION_HEAP(ION_CP_MM_HEAP_ID);
        DEBUG_PRINT_HIGH("ION ALLOC sec buf: size %d align %d flags %x",
                alloc_data->len, alloc_data->align,alloc_data->flags);
    } else {
        alloc_data->len = (size + (SZ_4K - 1)) & ~(SZ_4K - 1);
        alloc_data->align = SZ_4K;
        alloc_data->flags = (flag & ION_FLAG_CACHED ? ION_FLAG_CACHED : 0);
#ifdef MAX_RES_720P
    alloc_data->ION_HEAP_MASK = ION_HEAP(MEM_HEAP_ID);
#else
    alloc_data->ION_HEAP_MASK = (ION_HEAP(MEM_HEAP_ID) |
            ION_HEAP(ION_IOMMU_HEAP_ID));
#endif
        DEBUG_PRINT_HIGH("ION ALLOC unsec buf: size %d align %d flags %x",
                alloc_data->len, alloc_data->align,alloc_data->flags);
    }

    rc = ioctl(ion_device_fd,ION_IOC_ALLOC,alloc_data);
    if (rc || !alloc_data->handle) {
        DEBUG_PRINT_ERROR("ION ALLOC memory failed 0x%x", rc);
        alloc_data->handle = 0;
        close(ion_device_fd);
        ion_device_fd = -1;
        return ion_device_fd;
    }
    fd_data->handle = alloc_data->handle;
    rc = ioctl(ion_device_fd,ION_IOC_MAP,fd_data);
    if (rc) {
        DEBUG_PRINT_ERROR("ION MAP failed ");
        buf_ion_info.ion_alloc_data = *alloc_data;
        buf_ion_info.ion_device_fd = ion_device_fd;
        buf_ion_info.fd_ion_data = *fd_data;
        free_ion_memory(&buf_ion_info);
        fd_data->fd =-1;
        ion_device_fd =-1;
    }
    return ion_device_fd;
}

void omx_video::free_ion_memory(struct venc_ion *buf_ion_info)
{
    if (!buf_ion_info) {
        DEBUG_PRINT_ERROR("Invalid input to free_ion_memory");
        return;
    }
    if (ioctl(buf_ion_info->ion_device_fd,ION_IOC_FREE,
                &buf_ion_info->ion_alloc_data.handle)) {
        DEBUG_PRINT_ERROR("ION free failed ");
        return;
    }
    close(buf_ion_info->ion_device_fd);
    buf_ion_info->ion_alloc_data.handle = 0;
    buf_ion_info->ion_device_fd = -1;
    buf_ion_info->fd_ion_data.fd = -1;
}
#endif

#ifdef _ANDROID_ICS_
void omx_video::omx_release_meta_buffer(OMX_BUFFERHEADERTYPE *buffer)
{
    if (buffer && meta_mode_enable) {
        encoder_media_buffer_type *media_ptr;
        struct pmem Input_pmem;
        unsigned int index_pmem = 0;
        bool meta_error = false;

        index_pmem = (buffer - m_inp_mem_ptr);
        if (mUsesColorConversion &&
                (index_pmem < m_sInPortDef.nBufferCountActual)) {
            if (!dev_free_buf((&m_pInput_pmem[index_pmem]),PORT_INDEX_IN)) {
                DEBUG_PRINT_ERROR("omx_release_meta_buffer dev free failed");
            }
        } else {
            media_ptr = (encoder_media_buffer_type *) buffer->pBuffer;
            if (media_ptr && media_ptr->meta_handle) {
                if (media_ptr->buffer_type == kMetadataBufferTypeCameraSource &&
                        media_ptr->meta_handle->numFds == 1 &&
                        media_ptr->meta_handle->numInts >= 2) {
                    Input_pmem.fd = media_ptr->meta_handle->data[0];
                    Input_pmem.buffer = media_ptr;
                    Input_pmem.size = media_ptr->meta_handle->data[2];
                    Input_pmem.offset = media_ptr->meta_handle->data[1];
                    DEBUG_PRINT_LOW("EBD fd = %d, offset = %d, size = %d",Input_pmem.fd,
                            Input_pmem.offset,
                            Input_pmem.size);
                } else if (media_ptr->buffer_type == kMetadataBufferTypeGrallocSource) {
                    private_handle_t *handle = (private_handle_t *)media_ptr->meta_handle;
                    Input_pmem.buffer = media_ptr;
                    Input_pmem.fd = handle->fd;
                    Input_pmem.offset = 0;
                    Input_pmem.size = handle->size;
                } else {
                    meta_error = true;
                    DEBUG_PRINT_ERROR(" Meta Error set in EBD");
                }
                if (!meta_error)
                    meta_error = !dev_free_buf(&Input_pmem,PORT_INDEX_IN);
                if (meta_error) {
                    DEBUG_PRINT_ERROR(" Warning dev_free_buf failed flush value is %d",
                            input_flush_progress);
                }
            }
        }
    }
}
#endif
omx_video::omx_c2d_conv::omx_c2d_conv()
{
    c2dcc = NULL;
    mLibHandle = NULL;
    mConvertOpen = NULL;
    mConvertClose = NULL;
    src_format = NV12_128m;
    pthread_mutex_init(&c_lock, NULL);
}

bool omx_video::omx_c2d_conv::init()
{
    bool status = true;
    if (mLibHandle || mConvertOpen || mConvertClose) {
        DEBUG_PRINT_ERROR("omx_c2d_conv::init called twice");
        status = false;
    }
    if (status) {
        mLibHandle = dlopen("libc2dcolorconvert.so", RTLD_LAZY);
        if (mLibHandle) {
            mConvertOpen = (createC2DColorConverter_t *)
                dlsym(mLibHandle,"createC2DColorConverter");
            mConvertClose = (destroyC2DColorConverter_t *)
                dlsym(mLibHandle,"destroyC2DColorConverter");
            if (!mConvertOpen || !mConvertClose)
                status = false;
        } else
            status = false;
    }
    if (!status && mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
        mConvertOpen = NULL;
        mConvertClose = NULL;
    }
    return status;
}

bool omx_video::omx_c2d_conv::convert(int src_fd, void *src_base, void *src_viraddr,
        int dest_fd, void *dest_base, void *dest_viraddr)
{
    int result;
    if (!src_viraddr || !dest_viraddr || !c2dcc || !src_base || !dest_base) {
        DEBUG_PRINT_ERROR("Invalid arguments omx_c2d_conv::convert");
        return false;
    }
    pthread_mutex_lock(&c_lock);
    result =  c2dcc->convertC2D(src_fd, src_base, src_viraddr,
            dest_fd, dest_base, dest_viraddr);
    pthread_mutex_unlock(&c_lock);
    DEBUG_PRINT_LOW("Color convert status %d",result);
    return ((result < 0)?false:true);
}

bool omx_video::omx_c2d_conv::open(unsigned int height,unsigned int width,
        ColorConvertFormat src, ColorConvertFormat dest,unsigned int src_stride)
{
    bool status = false;
    pthread_mutex_lock(&c_lock);
    if (!c2dcc) {
        c2dcc = mConvertOpen(width, height, width, height,
                src,dest,0,src_stride);
        if (c2dcc) {
            src_format = src;
            status = true;
        } else
            DEBUG_PRINT_ERROR("mConvertOpen failed");
    }
    pthread_mutex_unlock(&c_lock);
    return status;
}

void omx_video::omx_c2d_conv::close()
{
    if (mLibHandle) {
        pthread_mutex_lock(&c_lock);
        if (mConvertClose && c2dcc)
            mConvertClose(c2dcc);
        pthread_mutex_unlock(&c_lock);
        c2dcc = NULL;
    }
}
omx_video::omx_c2d_conv::~omx_c2d_conv()
{
    DEBUG_PRINT_HIGH("Destroy C2D instance");
    if (mLibHandle) {
        if (mConvertClose && c2dcc) {
            pthread_mutex_lock(&c_lock);
            mConvertClose(c2dcc);
            pthread_mutex_unlock(&c_lock);
        }
        dlclose(mLibHandle);
    }
    c2dcc = NULL;
    mLibHandle = NULL;
    mConvertOpen = NULL;
    mConvertClose = NULL;
    pthread_mutex_destroy(&c_lock);
}

int omx_video::omx_c2d_conv::get_src_format()
{
    int format = -1;
    if (src_format == NV12_128m) {
        format = HAL_PIXEL_FORMAT_NV12_ENCODEABLE;
    } else if (src_format == RGBA8888) {
        format = HAL_PIXEL_FORMAT_RGBA_8888;
    }
    return format;
}

bool omx_video::omx_c2d_conv::get_buffer_size(int port,unsigned int &buf_size)
{
    int cret = 0;
    bool ret = false;
    C2DBuffReq bufferreq;
    if (c2dcc) {
        bufferreq.size = 0;
        pthread_mutex_lock(&c_lock);
        cret = c2dcc->getBuffReq(port,&bufferreq);
        pthread_mutex_unlock(&c_lock);
        DEBUG_PRINT_LOW("Status of getbuffer is %d", cret);
        ret = (cret)?false:true;
        buf_size = bufferreq.size;
    }
    return ret;
}

OMX_ERRORTYPE  omx_video::empty_this_buffer_opaque(OMX_IN OMX_HANDLETYPE hComp,
        OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    unsigned nBufIndex = 0;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    encoder_media_buffer_type *media_buffer;
    DEBUG_PRINT_LOW("ETBProxyOpaque: buffer[%p]", buffer);

    if (buffer == NULL) {
        DEBUG_PRINT_ERROR("ERROR: ETBProxyA: Invalid buffer[%p]",buffer);
        return OMX_ErrorBadParameter;
    }
    nBufIndex = buffer - meta_buffer_hdr;
    if (nBufIndex >= m_sInPortDef.nBufferCountActual) {
        DEBUG_PRINT_ERROR("ERROR: ETBProxyA: Invalid bufindex = %u",
                nBufIndex);
        return OMX_ErrorBadParameter;
    }
    media_buffer = (encoder_media_buffer_type *)buffer->pBuffer;
    private_handle_t *handle = (private_handle_t *)media_buffer->meta_handle;

    /*Enable following code once private handle color format is
      updated correctly*/

    if (buffer->nFilledLen > 0) {
        if (c2d_opened && handle->format != c2d_conv.get_src_format()) {
            c2d_conv.close();
            c2d_opened = false;
        }
        if (!c2d_opened) {
            if (handle->format == HAL_PIXEL_FORMAT_RGBA_8888) {
                mUsesColorConversion = true;
                DEBUG_PRINT_INFO("open Color conv for RGBA888 W: %u, H: %u",
                        (unsigned int)m_sInPortDef.format.video.nFrameWidth,
                        (unsigned int)m_sInPortDef.format.video.nFrameHeight);
                if (!c2d_conv.open(m_sInPortDef.format.video.nFrameHeight,
                            m_sInPortDef.format.video.nFrameWidth,
                            RGBA8888, NV12_128m, handle->width)) {
                    m_pCallbacks.EmptyBufferDone(hComp,m_app_data,buffer);
                    DEBUG_PRINT_ERROR("Color conv open failed");
                    return OMX_ErrorBadParameter;
                }
                c2d_opened = true;
#ifdef _MSM8974_
                if (!dev_set_format(handle->format))
                    DEBUG_PRINT_ERROR("cannot set color format for RGBA8888");
#endif
            } else if (handle->format != HAL_PIXEL_FORMAT_NV12_ENCODEABLE) {
                DEBUG_PRINT_ERROR("Incorrect color format");
                m_pCallbacks.EmptyBufferDone(hComp,m_app_data,buffer);
                return OMX_ErrorBadParameter;
            }
        }
    }
    if (input_flush_progress == true) {
        m_pCallbacks.EmptyBufferDone(hComp,m_app_data,buffer);
        DEBUG_PRINT_ERROR("ERROR: ETBProxyA: Input flush in progress");
        return OMX_ErrorNone;
    }

    if (!psource_frame) {
        psource_frame = buffer;
        ret = push_input_buffer(hComp);
    } else {
        if (!m_opq_meta_q.insert_entry((unsigned long)buffer,0,0)) {
            DEBUG_PRINT_ERROR("ERROR: ETBProxy: Queue is full");
            m_pCallbacks.EmptyBufferDone(hComp,m_app_data,buffer);
            ret = OMX_ErrorBadParameter;
        }
    }
    return ret;
}

OMX_ERRORTYPE omx_video::queue_meta_buffer(OMX_HANDLETYPE hComp,
        struct pmem &Input_pmem_info)
{

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    unsigned long address = 0,p2,id;

    DEBUG_PRINT_LOW("In queue Meta Buffer");
    if (!psource_frame || !pdest_frame) {
        DEBUG_PRINT_ERROR("convert_queue_buffer invalid params");
        return OMX_ErrorBadParameter;
    }

    if (psource_frame->nFilledLen > 0) {
        if (dev_use_buf(&Input_pmem_info,PORT_INDEX_IN,0) != true) {
            DEBUG_PRINT_ERROR("ERROR: in dev_use_buf");
            post_event ((unsigned long)psource_frame,0,OMX_COMPONENT_GENERATE_EBD);
            ret = OMX_ErrorBadParameter;
        }
    }

    if (ret == OMX_ErrorNone)
        ret = empty_this_buffer_proxy(hComp,psource_frame);

    if (ret == OMX_ErrorNone) {
        psource_frame = NULL;
        if (!psource_frame && m_opq_meta_q.m_size) {
            m_opq_meta_q.pop_entry(&address,&p2,&id);
            psource_frame = (OMX_BUFFERHEADERTYPE* ) address;
        }
    } else {
        // there has been an error and source frame has been scheduled for an EBD
        psource_frame = NULL;
    }
    return ret;
}

OMX_ERRORTYPE omx_video::convert_queue_buffer(OMX_HANDLETYPE hComp,
        struct pmem &Input_pmem_info,unsigned long &index)
{

    unsigned char *uva;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    unsigned long address = 0,p2,id;

    DEBUG_PRINT_LOW("In Convert and queue Meta Buffer");
    if (!psource_frame || !pdest_frame) {
        DEBUG_PRINT_ERROR("convert_queue_buffer invalid params");
        return OMX_ErrorBadParameter;
    }
    if (secure_session) {
        DEBUG_PRINT_ERROR("cannot convert buffer during secure session");
        return OMX_ErrorInvalidState;
    }

    if (!psource_frame->nFilledLen) {
        if(psource_frame->nFlags & OMX_BUFFERFLAG_EOS) {
            pdest_frame->nFilledLen = psource_frame->nFilledLen;
            pdest_frame->nTimeStamp = psource_frame->nTimeStamp;
            pdest_frame->nFlags = psource_frame->nFlags;
            DEBUG_PRINT_HIGH("Skipping color conversion for empty EOS Buffer "
                    "header=%p filled-len=%u", pdest_frame, (unsigned int)pdest_frame->nFilledLen);
        } else {
            pdest_frame->nOffset = 0;
            pdest_frame->nFilledLen = 0;
            pdest_frame->nTimeStamp = psource_frame->nTimeStamp;
            pdest_frame->nFlags = psource_frame->nFlags;
            DEBUG_PRINT_LOW("Buffer header %p Filled len size %u",
                    pdest_frame, (unsigned int)pdest_frame->nFilledLen);
        }
    } else {
        uva = (unsigned char *)mmap(NULL, Input_pmem_info.size,
                PROT_READ|PROT_WRITE,
                MAP_SHARED,Input_pmem_info.fd,0);
        if (uva == MAP_FAILED) {
            ret = OMX_ErrorBadParameter;
        } else {
            if (!c2d_conv.convert(Input_pmem_info.fd, uva, uva,
                        m_pInput_pmem[index].fd, pdest_frame->pBuffer, pdest_frame->pBuffer)) {
                DEBUG_PRINT_ERROR("Color Conversion failed");
                ret = OMX_ErrorBadParameter;
            } else {
                unsigned int buf_size = 0;
                if (!c2d_conv.get_buffer_size(C2D_OUTPUT,buf_size))
                    ret = OMX_ErrorBadParameter;
                else {
                    pdest_frame->nOffset = 0;
                    pdest_frame->nFilledLen = buf_size;
                    pdest_frame->nTimeStamp = psource_frame->nTimeStamp;
                    pdest_frame->nFlags = psource_frame->nFlags;
                    DEBUG_PRINT_LOW("Buffer header %p Filled len size %u",
                            pdest_frame, (unsigned int)pdest_frame->nFilledLen);
                }
            }
            munmap(uva,Input_pmem_info.size);
        }
    }
    if (dev_use_buf(&m_pInput_pmem[index],PORT_INDEX_IN,0) != true) {
        DEBUG_PRINT_ERROR("ERROR: in dev_use_buf");
        post_event ((unsigned long)pdest_frame,0,OMX_COMPONENT_GENERATE_EBD);
        ret = OMX_ErrorBadParameter;
    }
    if (ret == OMX_ErrorNone)
        ret = empty_this_buffer_proxy(hComp,pdest_frame);
    if (ret == OMX_ErrorNone) {
        m_pCallbacks.EmptyBufferDone(hComp ,m_app_data, psource_frame);
        psource_frame = NULL;
        pdest_frame = NULL;
        if (!psource_frame && m_opq_meta_q.m_size) {
            m_opq_meta_q.pop_entry(&address,&p2,&id);
            psource_frame = (OMX_BUFFERHEADERTYPE* ) address;
        }
        if (!pdest_frame && m_opq_pmem_q.m_size) {
            m_opq_pmem_q.pop_entry(&address,&p2,&id);
            pdest_frame = (OMX_BUFFERHEADERTYPE* ) address;
            DEBUG_PRINT_LOW("pdest_frame pop address is %p",pdest_frame);
        }
    } else {
        // there has been an error and source frame has been scheduled for an EBD
        psource_frame = NULL;
    }
    return ret;
}

OMX_ERRORTYPE omx_video::push_input_buffer(OMX_HANDLETYPE hComp)
{
    unsigned long address = 0,p2,id, index = 0;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!psource_frame && m_opq_meta_q.m_size) {
        m_opq_meta_q.pop_entry(&address,&p2,&id);
        psource_frame = (OMX_BUFFERHEADERTYPE* ) address;
    }
    if (!pdest_frame && m_opq_pmem_q.m_size) {
        m_opq_pmem_q.pop_entry(&address,&p2,&id);
        pdest_frame = (OMX_BUFFERHEADERTYPE* ) address;
    }
    while (psource_frame != NULL && pdest_frame != NULL &&
            ret == OMX_ErrorNone) {
        struct pmem Input_pmem_info;
        encoder_media_buffer_type *media_buffer;
        index = pdest_frame - m_inp_mem_ptr;
        if (index >= m_sInPortDef.nBufferCountActual) {
            DEBUG_PRINT_ERROR("Output buffer index is wrong %u act count %u",
                    (unsigned int)index, (unsigned int)m_sInPortDef.nBufferCountActual);
            return OMX_ErrorBadParameter;
        }

        //Meta-Buffer with empty filled-length can contain garbage handle
        //Some clients queue such buffers to signal EOS. Handle this case
        // separately by queueing an intermediate color-conversion buffer
        // and propagate the EOS.
        if (psource_frame->nFilledLen == 0 && (psource_frame->nFlags & OMX_BUFFERFLAG_EOS)) {
            return push_empty_eos_buffer(hComp, psource_frame);
        }
        media_buffer = (encoder_media_buffer_type *)psource_frame->pBuffer;
        /*Will enable to verify camcorder in current TIPS can be removed*/
        if (media_buffer->buffer_type == kMetadataBufferTypeCameraSource) {
            Input_pmem_info.buffer = media_buffer;
            Input_pmem_info.fd = media_buffer->meta_handle->data[0];
            Input_pmem_info.offset = media_buffer->meta_handle->data[1];
            Input_pmem_info.size = media_buffer->meta_handle->data[2];
            DEBUG_PRINT_LOW("ETB fd = %d, offset = %d, size = %d",Input_pmem_info.fd,
                    Input_pmem_info.offset,
                    Input_pmem_info.size);
            ret = queue_meta_buffer(hComp,Input_pmem_info);
        } else {
            private_handle_t *handle = (private_handle_t *)media_buffer->meta_handle;
            Input_pmem_info.buffer = media_buffer;
            Input_pmem_info.fd = handle->fd;
            Input_pmem_info.offset = 0;
            Input_pmem_info.size = handle->size;
            if (handle->format == HAL_PIXEL_FORMAT_RGBA_8888)
                ret = convert_queue_buffer(hComp,Input_pmem_info,index);
            else if (handle->format == HAL_PIXEL_FORMAT_NV12_ENCODEABLE)
                ret = queue_meta_buffer(hComp,Input_pmem_info);
            else
                ret = OMX_ErrorBadParameter;
        }
    }
    return ret;
}

OMX_ERRORTYPE omx_video::push_empty_eos_buffer(OMX_HANDLETYPE hComp,
        OMX_BUFFERHEADERTYPE* buffer) {
    OMX_BUFFERHEADERTYPE* opqBuf = NULL;
    OMX_ERRORTYPE retVal = OMX_ErrorNone;
    do {
        if (pdest_frame) {
            //[1] use a checked out conversion buffer, if one is available
            opqBuf = pdest_frame;
            pdest_frame = NULL;
        } else if (m_opq_pmem_q.m_size) {
            //[2] else pop out one from the queue, if available
            unsigned long address = 0, p2, id;
            m_opq_pmem_q.pop_entry(&address,&p2,&id);
            opqBuf = (OMX_BUFFERHEADERTYPE* ) address;
        }
        unsigned index = opqBuf - m_inp_mem_ptr;
        if (!opqBuf || index >= m_sInPortDef.nBufferCountActual) {
            DEBUG_PRINT_ERROR("push_empty_eos_buffer: Could not find a "
                    "color-conversion buffer to queue ! defer until available");
            //[3] else, returning back will defer calling this function again
            //until a conversion buffer is returned by the encoder and also
            //hold on to the client's buffer
            return OMX_ErrorNone;
        }
        struct pmem Input_pmem_info;
        Input_pmem_info.buffer = opqBuf;
        Input_pmem_info.fd = m_pInput_pmem[index].fd;
        Input_pmem_info.offset = 0;
        Input_pmem_info.size = m_pInput_pmem[index].size;

        if (dev_use_buf(&Input_pmem_info, PORT_INDEX_IN, 0) != true) {
            DEBUG_PRINT_ERROR("ERROR: in dev_use_buf for empty eos buffer");
            retVal = OMX_ErrorBadParameter;
            break;
        }

        //Queue with null pBuffer, as pBuffer in client's hdr can be junk
        //Clone the color-conversion buffer to avoid overwriting original buffer
        OMX_BUFFERHEADERTYPE emptyEosBufHdr;
        memcpy(&emptyEosBufHdr, opqBuf, sizeof(OMX_BUFFERHEADERTYPE));
        emptyEosBufHdr.nFilledLen = 0;
        emptyEosBufHdr.nTimeStamp = buffer->nTimeStamp;
        emptyEosBufHdr.nFlags = buffer->nFlags;
        emptyEosBufHdr.pBuffer = NULL;
        if (dev_empty_buf(&emptyEosBufHdr, 0, index, m_pInput_pmem[index].fd) != true) {
            DEBUG_PRINT_ERROR("ERROR: in dev_empty_buf for empty eos buffer");
            dev_free_buf(&Input_pmem_info, PORT_INDEX_IN);
            retVal = OMX_ErrorBadParameter;
            break;
        }
        mEmptyEosBuffer = opqBuf;
    } while(false);

    //return client's buffer regardless since intermediate color-conversion
    //buffer is sent to the the encoder
    m_pCallbacks.EmptyBufferDone(hComp, m_app_data, buffer);
    --pending_input_buffers;
    return retVal;
}

