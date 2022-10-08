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

#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <cam_semaphore.h>

#include "mm_camera_dbg.h"
#include "mm_camera_interface.h"
#include "mm_camera.h"

extern mm_camera_obj_t* mm_camera_util_get_camera_by_handler(uint32_t cam_handler);
extern mm_channel_t * mm_camera_util_get_channel_by_handler(mm_camera_obj_t * cam_obj,
                                                            uint32_t handler);

/* internal function declare goes here */
int32_t mm_channel_qbuf(mm_channel_t *my_obj,
                        mm_camera_buf_def_t *buf);
int32_t mm_channel_init(mm_channel_t *my_obj,
                        mm_camera_channel_attr_t *attr,
                        mm_camera_buf_notify_t channel_cb,
                        void *userdata);
void mm_channel_release(mm_channel_t *my_obj);
uint32_t mm_channel_add_stream(mm_channel_t *my_obj);
int32_t mm_channel_del_stream(mm_channel_t *my_obj,
                                   uint32_t stream_id);
int32_t mm_channel_config_stream(mm_channel_t *my_obj,
                                 uint32_t stream_id,
                                 mm_camera_stream_config_t *config);
int32_t mm_channel_get_bundle_info(mm_channel_t *my_obj,
                                   cam_bundle_config_t *bundle_info);
int32_t mm_channel_start(mm_channel_t *my_obj);
int32_t mm_channel_stop(mm_channel_t *my_obj);
int32_t mm_channel_request_super_buf(mm_channel_t *my_obj,
                                     uint32_t num_buf_requested);
int32_t mm_channel_cancel_super_buf_request(mm_channel_t *my_obj);
int32_t mm_channel_flush_super_buf_queue(mm_channel_t *my_obj,
                                         uint32_t frame_idx);
int32_t mm_channel_config_notify_mode(mm_channel_t *my_obj,
                                      mm_camera_super_buf_notify_mode_t notify_mode);
int32_t mm_channel_superbuf_flush(mm_channel_t* my_obj, mm_channel_queue_t * queue);
int32_t mm_channel_set_stream_parm(mm_channel_t *my_obj,
                                   mm_evt_paylod_set_get_stream_parms_t *payload);
int32_t mm_channel_get_stream_parm(mm_channel_t *my_obj,
                                   mm_evt_paylod_set_get_stream_parms_t *payload);
int32_t mm_channel_do_stream_action(mm_channel_t *my_obj,
                                    mm_evt_paylod_do_stream_action_t *payload);
int32_t mm_channel_map_stream_buf(mm_channel_t *my_obj,
                                  mm_evt_paylod_map_stream_buf_t *payload);
int32_t mm_channel_unmap_stream_buf(mm_channel_t *my_obj,
                                    mm_evt_paylod_unmap_stream_buf_t *payload);

/* state machine function declare */
int32_t mm_channel_fsm_fn_notused(mm_channel_t *my_obj,
                          mm_channel_evt_type_t evt,
                          void * in_val,
                          void * out_val);
int32_t mm_channel_fsm_fn_stopped(mm_channel_t *my_obj,
                          mm_channel_evt_type_t evt,
                          void * in_val,
                          void * out_val);
int32_t mm_channel_fsm_fn_active(mm_channel_t *my_obj,
                          mm_channel_evt_type_t evt,
                          void * in_val,
                          void * out_val);
int32_t mm_channel_fsm_fn_paused(mm_channel_t *my_obj,
                          mm_channel_evt_type_t evt,
                          void * in_val,
                          void * out_val);

/* channel super queue functions */
int32_t mm_channel_superbuf_queue_init(mm_channel_queue_t * queue);
int32_t mm_channel_superbuf_queue_deinit(mm_channel_queue_t * queue);
int32_t mm_channel_superbuf_comp_and_enqueue(mm_channel_t *ch_obj,
                                             mm_channel_queue_t * queue,
                                             mm_camera_buf_info_t *buf);
mm_channel_queue_node_t* mm_channel_superbuf_dequeue(mm_channel_queue_t * queue);
int32_t mm_channel_superbuf_bufdone_overflow(mm_channel_t *my_obj,
                                             mm_channel_queue_t *queue);
int32_t mm_channel_superbuf_skip(mm_channel_t *my_obj,
                                 mm_channel_queue_t *queue);

/*===========================================================================
 * FUNCTION   : mm_channel_util_get_stream_by_handler
 *
 * DESCRIPTION: utility function to get a stream object from its handle
 *
 * PARAMETERS :
 *   @cam_obj: ptr to a channel object
 *   @handler: stream handle
 *
 * RETURN     : ptr to a stream object.
 *              NULL if failed.
 *==========================================================================*/
mm_stream_t * mm_channel_util_get_stream_by_handler(
                                    mm_channel_t * ch_obj,
                                    uint32_t handler)
{
    int i;
    mm_stream_t *s_obj = NULL;
    for(i = 0; i < MAX_STREAM_NUM_IN_BUNDLE; i++) {
        if ((MM_STREAM_STATE_NOTUSED != ch_obj->streams[i].state) &&
            (handler == ch_obj->streams[i].my_hdl)) {
            s_obj = &ch_obj->streams[i];
            break;
        }
    }
    return s_obj;
}

/*===========================================================================
 * FUNCTION   : mm_channel_dispatch_super_buf
 *
 * DESCRIPTION: dispatch super buffer of bundle to registered user
 *
 * PARAMETERS :
 *   @cmd_cb  : ptr storing matched super buf information
 *   @userdata: user data ptr
 *
 * RETURN     : none
 *==========================================================================*/
static void mm_channel_dispatch_super_buf(mm_camera_cmdcb_t *cmd_cb,
                                          void* user_data)
{
    mm_channel_t * my_obj = (mm_channel_t *)user_data;

    if (NULL == my_obj) {
        return;
    }

    if (MM_CAMERA_CMD_TYPE_SUPER_BUF_DATA_CB != cmd_cb->cmd_type) {
        CDBG_ERROR("%s: Wrong cmd_type (%d) for super buf dataCB",
                   __func__, cmd_cb->cmd_type);
        return;
    }

    if (my_obj->bundle.super_buf_notify_cb) {
        my_obj->bundle.super_buf_notify_cb(&cmd_cb->u.superbuf, my_obj->bundle.user_data);
    }
}

/*===========================================================================
 * FUNCTION   : mm_channel_process_stream_buf
 *
 * DESCRIPTION: handle incoming buffer from stream in a bundle. In this function,
 *              matching logic will be performed on incoming stream frames.
 *              Will depends on the bundle attribute, either storing matched frames
 *              in the superbuf queue, or sending matched superbuf frames to upper
 *              layer through registered callback.
 *
 * PARAMETERS :
 *   @cmd_cb  : ptr storing matched super buf information
 *   @userdata: user data ptr
 *
 * RETURN     : none
 *==========================================================================*/
static void mm_channel_process_stream_buf(mm_camera_cmdcb_t * cmd_cb,
                                          void *user_data)
{
    mm_camera_super_buf_notify_mode_t notify_mode;
    mm_channel_queue_node_t *node = NULL;
    mm_channel_t *ch_obj = (mm_channel_t *)user_data;
    if (NULL == ch_obj) {
        return;
    }

    if (MM_CAMERA_CMD_TYPE_DATA_CB  == cmd_cb->cmd_type) {
        /* comp_and_enqueue */
        mm_channel_superbuf_comp_and_enqueue(
                        ch_obj,
                        &ch_obj->bundle.superbuf_queue,
                        &cmd_cb->u.buf);
    } else if (MM_CAMERA_CMD_TYPE_REQ_DATA_CB  == cmd_cb->cmd_type) {
        /* skip frames if needed */
        ch_obj->pending_cnt = cmd_cb->u.req_buf.num_buf_requested;
        mm_channel_superbuf_skip(ch_obj, &ch_obj->bundle.superbuf_queue);
    } else if (MM_CAMERA_CMD_TYPE_CONFIG_NOTIFY == cmd_cb->cmd_type) {
           ch_obj->bundle.superbuf_queue.attr.notify_mode = cmd_cb->u.notify_mode;
    } else if (MM_CAMERA_CMD_TYPE_FLUSH_QUEUE  == cmd_cb->cmd_type) {
        ch_obj->bundle.superbuf_queue.expected_frame_id = cmd_cb->u.frame_idx;
        mm_channel_superbuf_flush(ch_obj, &ch_obj->bundle.superbuf_queue);
        return;
    }
    notify_mode = ch_obj->bundle.superbuf_queue.attr.notify_mode;

    /* bufdone for overflowed bufs */
    mm_channel_superbuf_bufdone_overflow(ch_obj, &ch_obj->bundle.superbuf_queue);

    /* dispatch frame if pending_cnt>0 or is in continuous streaming mode */
    while ( (ch_obj->pending_cnt > 0) ||
            (MM_CAMERA_SUPER_BUF_NOTIFY_CONTINUOUS == notify_mode) ) {

        /* dequeue */
        node = mm_channel_superbuf_dequeue(&ch_obj->bundle.superbuf_queue);
        if (NULL != node) {
            /* decrease pending_cnt */
            CDBG("%s: Super Buffer received, Call client callback, pending_cnt=%d",
                 __func__, ch_obj->pending_cnt);
            if (MM_CAMERA_SUPER_BUF_NOTIFY_BURST == notify_mode) {
                ch_obj->pending_cnt--;
            }

            /* dispatch superbuf */
            if (NULL != ch_obj->bundle.super_buf_notify_cb) {
                uint8_t i;
                mm_camera_cmdcb_t* cb_node = NULL;

                CDBG("%s: Send superbuf to HAL, pending_cnt=%d",
                     __func__, ch_obj->pending_cnt);

                /* send cam_sem_post to wake up cb thread to dispatch super buffer */
                cb_node = (mm_camera_cmdcb_t *)malloc(sizeof(mm_camera_cmdcb_t));
                if (NULL != cb_node) {
                    memset(cb_node, 0, sizeof(mm_camera_cmdcb_t));
                    cb_node->cmd_type = MM_CAMERA_CMD_TYPE_SUPER_BUF_DATA_CB;
                    cb_node->u.superbuf.num_bufs = node->num_of_bufs;
                    for (i=0; i<node->num_of_bufs; i++) {
                        cb_node->u.superbuf.bufs[i] = node->super_buf[i].buf;
                    }
                    cb_node->u.superbuf.camera_handle = ch_obj->cam_obj->my_hdl;
                    cb_node->u.superbuf.ch_id = ch_obj->my_hdl;

                    /* enqueue to cb thread */
                    cam_queue_enq(&(ch_obj->cb_thread.cmd_queue), cb_node);

                    /* wake up cb thread */
                    cam_sem_post(&(ch_obj->cb_thread.cmd_sem));
                } else {
                    CDBG_ERROR("%s: No memory for mm_camera_node_t", __func__);
                    /* buf done with the nonuse super buf */
                    for (i=0; i<node->num_of_bufs; i++) {
                        mm_channel_qbuf(ch_obj, node->super_buf[i].buf);
                    }
                }
            } else {
                /* buf done with the nonuse super buf */
                uint8_t i;
                for (i=0; i<node->num_of_bufs; i++) {
                    mm_channel_qbuf(ch_obj, node->super_buf[i].buf);
                }
            }
            free(node);
        } else {
            /* no superbuf avail, break the loop */
            break;
        }
    }
}

/*===========================================================================
 * FUNCTION   : mm_channel_fsm_fn
 *
 * DESCRIPTION: channel finite state machine entry function. Depends on channel
 *              state, incoming event will be handled differently.
 *
 * PARAMETERS :
 *   @my_obj   : ptr to a channel object
 *   @evt      : channel event to be processed
 *   @in_val   : input event payload. Can be NULL if not needed.
 *   @out_val  : output payload, Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_fsm_fn(mm_channel_t *my_obj,
                          mm_channel_evt_type_t evt,
                          void * in_val,
                          void * out_val)
{
    int32_t rc = -1;

    CDBG("%s : E state = %d", __func__, my_obj->state);
    switch (my_obj->state) {
    case MM_CHANNEL_STATE_NOTUSED:
        rc = mm_channel_fsm_fn_notused(my_obj, evt, in_val, out_val);
        break;
    case MM_CHANNEL_STATE_STOPPED:
        rc = mm_channel_fsm_fn_stopped(my_obj, evt, in_val, out_val);
        break;
    case MM_CHANNEL_STATE_ACTIVE:
        rc = mm_channel_fsm_fn_active(my_obj, evt, in_val, out_val);
        break;
    case MM_CHANNEL_STATE_PAUSED:
        rc = mm_channel_fsm_fn_paused(my_obj, evt, in_val, out_val);
        break;
    default:
        CDBG("%s: Not a valid state (%d)", __func__, my_obj->state);
        break;
    }

    /* unlock ch_lock */
    pthread_mutex_unlock(&my_obj->ch_lock);
    CDBG("%s : X rc = %d", __func__, rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_fsm_fn_notused
 *
 * DESCRIPTION: channel finite state machine function to handle event
 *              in NOT_USED state.
 *
 * PARAMETERS :
 *   @my_obj   : ptr to a channel object
 *   @evt      : channel event to be processed
 *   @in_val   : input event payload. Can be NULL if not needed.
 *   @out_val  : output payload, Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_fsm_fn_notused(mm_channel_t *my_obj,
                                  mm_channel_evt_type_t evt,
                                  void * in_val,
                                  void * out_val)
{
    int32_t rc = -1;

    switch (evt) {
    default:
        CDBG_ERROR("%s: invalid state (%d) for evt (%d), in(%p), out(%p)",
                   __func__, my_obj->state, evt, in_val, out_val);
        break;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_fsm_fn_stopped
 *
 * DESCRIPTION: channel finite state machine function to handle event
 *              in STOPPED state.
 *
 * PARAMETERS :
 *   @my_obj   : ptr to a channel object
 *   @evt      : channel event to be processed
 *   @in_val   : input event payload. Can be NULL if not needed.
 *   @out_val  : output payload, Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_fsm_fn_stopped(mm_channel_t *my_obj,
                                  mm_channel_evt_type_t evt,
                                  void * in_val,
                                  void * out_val)
{
    int32_t rc = 0;
    CDBG("%s : E evt = %d", __func__, evt);
    switch (evt) {
    case MM_CHANNEL_EVT_ADD_STREAM:
        {
            uint32_t s_hdl = 0;
            s_hdl = mm_channel_add_stream(my_obj);
            *((uint32_t*)out_val) = s_hdl;
            rc = 0;
        }
        break;
    case MM_CHANNEL_EVT_DEL_STREAM:
        {
            uint32_t s_id = (uint32_t)in_val;
            rc = mm_channel_del_stream(my_obj, s_id);
        }
        break;
    case MM_CHANNEL_EVT_START:
        {
            rc = mm_channel_start(my_obj);
            /* first stream started in stopped state
             * move to active state */
            if (0 == rc) {
                my_obj->state = MM_CHANNEL_STATE_ACTIVE;
            }
        }
        break;
    case MM_CHANNEL_EVT_CONFIG_STREAM:
        {
            mm_evt_paylod_config_stream_t *payload =
                (mm_evt_paylod_config_stream_t *)in_val;
            rc = mm_channel_config_stream(my_obj,
                                          payload->stream_id,
                                          payload->config);
        }
        break;
    case MM_CHANNEL_EVT_GET_BUNDLE_INFO:
        {
            cam_bundle_config_t *payload =
                (cam_bundle_config_t *)in_val;
            rc = mm_channel_get_bundle_info(my_obj, payload);
        }
        break;
    case MM_CHANNEL_EVT_DELETE:
        {
            mm_channel_release(my_obj);
            rc = 0;
        }
        break;
    case MM_CHANNEL_EVT_SET_STREAM_PARM:
        {
            mm_evt_paylod_set_get_stream_parms_t *payload =
                (mm_evt_paylod_set_get_stream_parms_t *)in_val;
            rc = mm_channel_set_stream_parm(my_obj, payload);
        }
        break;
    case MM_CHANNEL_EVT_GET_STREAM_PARM:
        {
            mm_evt_paylod_set_get_stream_parms_t *payload =
                (mm_evt_paylod_set_get_stream_parms_t *)in_val;
            rc = mm_channel_get_stream_parm(my_obj, payload);
        }
        break;
    case MM_CHANNEL_EVT_DO_STREAM_ACTION:
        {
            mm_evt_paylod_do_stream_action_t *payload =
                (mm_evt_paylod_do_stream_action_t *)in_val;
            rc = mm_channel_do_stream_action(my_obj, payload);
        }
        break;
    case MM_CHANNEL_EVT_MAP_STREAM_BUF:
        {
            mm_evt_paylod_map_stream_buf_t *payload =
                (mm_evt_paylod_map_stream_buf_t *)in_val;
            rc = mm_channel_map_stream_buf(my_obj, payload);
        }
        break;
    case MM_CHANNEL_EVT_UNMAP_STREAM_BUF:
        {
            mm_evt_paylod_unmap_stream_buf_t *payload =
                (mm_evt_paylod_unmap_stream_buf_t *)in_val;
            rc = mm_channel_unmap_stream_buf(my_obj, payload);
        }
        break;
    default:
        CDBG_ERROR("%s: invalid state (%d) for evt (%d)",
                   __func__, my_obj->state, evt);
        break;
    }
    CDBG("%s : E rc = %d", __func__, rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_fsm_fn_active
 *
 * DESCRIPTION: channel finite state machine function to handle event
 *              in ACTIVE state.
 *
 * PARAMETERS :
 *   @my_obj   : ptr to a channel object
 *   @evt      : channel event to be processed
 *   @in_val   : input event payload. Can be NULL if not needed.
 *   @out_val  : output payload, Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_fsm_fn_active(mm_channel_t *my_obj,
                          mm_channel_evt_type_t evt,
                          void * in_val,
                          void * out_val)
{
    int32_t rc = 0;

    CDBG("%s : E evt = %d", __func__, evt);
    switch (evt) {
    case MM_CHANNEL_EVT_STOP:
        {
            rc = mm_channel_stop(my_obj);
            my_obj->state = MM_CHANNEL_STATE_STOPPED;
        }
        break;
    case MM_CHANNEL_EVT_REQUEST_SUPER_BUF:
        {
            uint32_t num_buf_requested = (uint32_t)in_val;
            rc = mm_channel_request_super_buf(my_obj, num_buf_requested);
        }
        break;
    case MM_CHANNEL_EVT_CANCEL_REQUEST_SUPER_BUF:
        {
            rc = mm_channel_cancel_super_buf_request(my_obj);
        }
        break;
    case MM_CHANNEL_EVT_FLUSH_SUPER_BUF_QUEUE:
        {
            uint32_t frame_idx = (uint32_t)in_val;
            rc = mm_channel_flush_super_buf_queue(my_obj, frame_idx);
        }
        break;
    case MM_CHANNEL_EVT_CONFIG_NOTIFY_MODE:
        {
            mm_camera_super_buf_notify_mode_t notify_mode = ( mm_camera_super_buf_notify_mode_t ) in_val;
            rc = mm_channel_config_notify_mode(my_obj, notify_mode);
        }
        break;
    case MM_CHANNEL_EVT_SET_STREAM_PARM:
        {
            mm_evt_paylod_set_get_stream_parms_t *payload =
                (mm_evt_paylod_set_get_stream_parms_t *)in_val;
            rc = mm_channel_set_stream_parm(my_obj, payload);
        }
        break;
    case MM_CHANNEL_EVT_GET_STREAM_PARM:
        {
            mm_evt_paylod_set_get_stream_parms_t *payload =
                (mm_evt_paylod_set_get_stream_parms_t *)in_val;
            rc = mm_channel_get_stream_parm(my_obj, payload);
        }
        break;
    case MM_CHANNEL_EVT_DO_STREAM_ACTION:
        {
            mm_evt_paylod_do_stream_action_t *payload =
                (mm_evt_paylod_do_stream_action_t *)in_val;
            rc = mm_channel_do_stream_action(my_obj, payload);
        }
        break;
    case MM_CHANNEL_EVT_MAP_STREAM_BUF:
        {
            mm_evt_paylod_map_stream_buf_t *payload =
                (mm_evt_paylod_map_stream_buf_t *)in_val;
            if (payload != NULL &&
                payload->buf_type == CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF) {
                rc = mm_channel_map_stream_buf(my_obj, payload);
            } else {
                CDBG_ERROR("%s: cannot map regualr stream buf in active state", __func__);
            }
        }
        break;
    case MM_CHANNEL_EVT_UNMAP_STREAM_BUF:
        {
            mm_evt_paylod_unmap_stream_buf_t *payload =
                (mm_evt_paylod_unmap_stream_buf_t *)in_val;
            if (payload != NULL &&
                payload->buf_type == CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF) {
                rc = mm_channel_unmap_stream_buf(my_obj, payload);
            } else {
                CDBG_ERROR("%s: cannot unmap regualr stream buf in active state", __func__);
            }
        }
        break;
    default:
        CDBG_ERROR("%s: invalid state (%d) for evt (%d), in(%p), out(%p)",
                   __func__, my_obj->state, evt, in_val, out_val);
        break;
    }
    CDBG("%s : X rc = %d", __func__, rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_fsm_fn_paused
 *
 * DESCRIPTION: channel finite state machine function to handle event
 *              in PAUSED state.
 *
 * PARAMETERS :
 *   @my_obj   : ptr to a channel object
 *   @evt      : channel event to be processed
 *   @in_val   : input event payload. Can be NULL if not needed.
 *   @out_val  : output payload, Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_fsm_fn_paused(mm_channel_t *my_obj,
                          mm_channel_evt_type_t evt,
                          void * in_val,
                          void * out_val)
{
    int32_t rc = 0;

    /* currently we are not supporting pause/resume channel */
    CDBG_ERROR("%s: invalid state (%d) for evt (%d), in(%p), out(%p)",
               __func__, my_obj->state, evt, in_val, out_val);

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_init
 *
 * DESCRIPTION: initialize a channel
 *
 * PARAMETERS :
 *   @my_obj       : channel object be to initialized
 *   @attr         : bundle attribute of the channel if needed
 *   @channel_cb   : callback function for bundle data notify
 *   @userdata     : user data ptr
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : if no bundle data notify is needed, meaning each stream in the
 *              channel will have its own stream data notify callback, then
 *              attr, channel_cb, and userdata can be NULL. In this case,
 *              no matching logic will be performed in channel for the bundling.
 *==========================================================================*/
int32_t mm_channel_init(mm_channel_t *my_obj,
                        mm_camera_channel_attr_t *attr,
                        mm_camera_buf_notify_t channel_cb,
                        void *userdata)
{
    int32_t rc = 0;

    my_obj->bundle.super_buf_notify_cb = channel_cb;
    my_obj->bundle.user_data = userdata;
    if (NULL != attr) {
        my_obj->bundle.superbuf_queue.attr = *attr;
    }

    CDBG("%s : Launch data poll thread in channel open", __func__);
    mm_camera_poll_thread_launch(&my_obj->poll_thread[0],
                                 MM_CAMERA_POLL_TYPE_DATA);

    /* change state to stopped state */
    my_obj->state = MM_CHANNEL_STATE_STOPPED;
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_release
 *
 * DESCRIPTION: release a channel resource. Channel state will move to UNUSED
 *              state after this call.
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *
 * RETURN     : none
 *==========================================================================*/
void mm_channel_release(mm_channel_t *my_obj)
{
    /* stop data poll thread */
    mm_camera_poll_thread_release(&my_obj->poll_thread[0]);

    /* change state to notused state */
    my_obj->state = MM_CHANNEL_STATE_NOTUSED;
}

/*===========================================================================
 * FUNCTION   : mm_channel_add_stream
 *
 * DESCRIPTION: add a stream into the channel
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *
 * RETURN     : uint32_t type of stream handle
 *              0  -- invalid stream handle, meaning the op failed
 *              >0 -- successfully added a stream with a valid handle
 *==========================================================================*/
uint32_t mm_channel_add_stream(mm_channel_t *my_obj)
{
    int32_t rc = 0;
    uint8_t idx = 0;
    uint32_t s_hdl = 0;
    mm_stream_t *stream_obj = NULL;

    CDBG("%s : E", __func__);
    /* check available stream */
    for (idx = 0; idx < MAX_STREAM_NUM_IN_BUNDLE; idx++) {
        if (MM_STREAM_STATE_NOTUSED == my_obj->streams[idx].state) {
            stream_obj = &my_obj->streams[idx];
            break;
        }
    }
    if (NULL == stream_obj) {
        CDBG_ERROR("%s: streams reach max, no more stream allowed to add", __func__);
        return s_hdl;
    }

    /* initialize stream object */
    memset(stream_obj, 0, sizeof(mm_stream_t));
    stream_obj->my_hdl = mm_camera_util_generate_handler(idx);
    stream_obj->ch_obj = my_obj;
    pthread_mutex_init(&stream_obj->buf_lock, NULL);
    pthread_mutex_init(&stream_obj->cb_lock, NULL);
    stream_obj->state = MM_STREAM_STATE_INITED;

    /* acquire stream */
    rc = mm_stream_fsm_fn(stream_obj, MM_STREAM_EVT_ACQUIRE, NULL, NULL);
    if (0 == rc) {
        s_hdl = stream_obj->my_hdl;
    } else {
        /* error during acquire, de-init */
        pthread_mutex_destroy(&stream_obj->buf_lock);
        pthread_mutex_destroy(&stream_obj->cb_lock);
        memset(stream_obj, 0, sizeof(mm_stream_t));
    }
    CDBG("%s : stream handle = %d", __func__, s_hdl);
    return s_hdl;
}

/*===========================================================================
 * FUNCTION   : mm_channel_del_stream
 *
 * DESCRIPTION: delete a stream from the channel bu its handle
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *   @stream_id    : stream handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : assume steam is stooped before it can be deleted
 *==========================================================================*/
int32_t mm_channel_del_stream(mm_channel_t *my_obj,
                              uint32_t stream_id)
{
    int rc = -1;
    mm_stream_t * stream_obj = NULL;
    stream_obj = mm_channel_util_get_stream_by_handler(my_obj, stream_id);

    if (NULL == stream_obj) {
        CDBG_ERROR("%s :Invalid Stream Object for stream_id = %d",
                   __func__, stream_id);
        return rc;
    }

    rc = mm_stream_fsm_fn(stream_obj,
                          MM_STREAM_EVT_RELEASE,
                          NULL,
                          NULL);

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_config_stream
 *
 * DESCRIPTION: configure a stream
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *   @stream_id    : stream handle
 *   @config       : stream configuration
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_config_stream(mm_channel_t *my_obj,
                                   uint32_t stream_id,
                                   mm_camera_stream_config_t *config)
{
    int rc = -1;
    mm_stream_t * stream_obj = NULL;
    CDBG("%s : E stream ID = %d", __func__, stream_id);
    stream_obj = mm_channel_util_get_stream_by_handler(my_obj, stream_id);

    if (NULL == stream_obj) {
        CDBG_ERROR("%s :Invalid Stream Object for stream_id = %d", __func__, stream_id);
        return rc;
    }

    /* set stream fmt */
    rc = mm_stream_fsm_fn(stream_obj,
                          MM_STREAM_EVT_SET_FMT,
                          (void *)config,
                          NULL);
    CDBG("%s : X rc = %d",__func__,rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_get_bundle_info
 *
 * DESCRIPTION: query bundle info of the channel, which should include all
 *              streams within this channel
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *   @bundle_info  : bundle info to be filled in
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_get_bundle_info(mm_channel_t *my_obj,
                                   cam_bundle_config_t *bundle_info)
{
    int i;
    mm_stream_t *s_obj = NULL;
    int32_t rc = 0;

    memset(bundle_info, 0, sizeof(cam_bundle_config_t));
    bundle_info->bundle_id = my_obj->my_hdl;
    bundle_info->num_of_streams = 0;
    for (i = 0; i < MAX_STREAM_NUM_IN_BUNDLE; i++) {
        if (my_obj->streams[i].my_hdl > 0) {
            s_obj = mm_channel_util_get_stream_by_handler(my_obj,
                                                          my_obj->streams[i].my_hdl);
            if (NULL != s_obj) {
                if (CAM_STREAM_TYPE_METADATA != s_obj->stream_info->stream_type) {
                    bundle_info->stream_ids[bundle_info->num_of_streams++] =
                                                        s_obj->server_stream_id;
                }
            } else {
                CDBG_ERROR("%s: cannot find stream obj (%d) by handler (%d)",
                           __func__, i, my_obj->streams[i].my_hdl);
                rc = -1;
                break;
            }
        }
    }
    if (rc != 0) {
        /* error, reset to 0 */
        memset(bundle_info, 0, sizeof(cam_bundle_config_t));
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_start
 *
 * DESCRIPTION: start a channel, which will start all streams in the channel
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_start(mm_channel_t *my_obj)
{
    int32_t rc = 0;
    int i, j;
    mm_stream_t *s_objs[MAX_STREAM_NUM_IN_BUNDLE] = {NULL};
    uint8_t num_streams_to_start = 0;
    mm_stream_t *s_obj = NULL;
    int meta_stream_idx = 0;

    for (i = 0; i < MAX_STREAM_NUM_IN_BUNDLE; i++) {
        if (my_obj->streams[i].my_hdl > 0) {
            s_obj = mm_channel_util_get_stream_by_handler(my_obj,
                                                          my_obj->streams[i].my_hdl);
            if (NULL != s_obj) {
                /* remember meta data stream index */
                if (s_obj->stream_info->stream_type == CAM_STREAM_TYPE_METADATA) {
                    meta_stream_idx = num_streams_to_start;
                }
                s_objs[num_streams_to_start++] = s_obj;
            }
        }
    }

    if (meta_stream_idx > 0 ) {
        /* always start meta data stream first, so switch the stream object with the first one */
        s_obj = s_objs[0];
        s_objs[0] = s_objs[meta_stream_idx];
        s_objs[meta_stream_idx] = s_obj;
    }

    if (NULL != my_obj->bundle.super_buf_notify_cb) {
        /* need to send up cb, therefore launch thread */
        /* init superbuf queue */
        mm_channel_superbuf_queue_init(&my_obj->bundle.superbuf_queue);
        my_obj->bundle.superbuf_queue.num_streams = num_streams_to_start;
        my_obj->bundle.superbuf_queue.expected_frame_id = 0;

        for (i = 0; i < num_streams_to_start; i++) {
            /* set bundled flag to streams */
            s_objs[i]->is_bundled = 1;
            /* init bundled streams to invalid value -1 */
            my_obj->bundle.superbuf_queue.bundled_streams[i] = s_objs[i]->my_hdl;
        }

        /* launch cb thread for dispatching super buf through cb */
        mm_camera_cmd_thread_launch(&my_obj->cb_thread,
                                    mm_channel_dispatch_super_buf,
                                    (void*)my_obj);

        /* launch cmd thread for super buf dataCB */
        mm_camera_cmd_thread_launch(&my_obj->cmd_thread,
                                    mm_channel_process_stream_buf,
                                    (void*)my_obj);

        /* set flag to TRUE */
        my_obj->bundle.is_active = TRUE;
    }

    for (i = 0; i < num_streams_to_start; i++) {
        /* all streams within a channel should be started at the same time */
        if (s_objs[i]->state == MM_STREAM_STATE_ACTIVE) {
            CDBG_ERROR("%s: stream already started idx(%d)", __func__, i);
            rc = -1;
            break;
        }

        /* allocate buf */
        rc = mm_stream_fsm_fn(s_objs[i],
                              MM_STREAM_EVT_GET_BUF,
                              NULL,
                              NULL);
        if (0 != rc) {
            CDBG_ERROR("%s: get buf failed at idx(%d)", __func__, i);
            break;
        }

        /* reg buf */
        rc = mm_stream_fsm_fn(s_objs[i],
                              MM_STREAM_EVT_REG_BUF,
                              NULL,
                              NULL);
        if (0 != rc) {
            CDBG_ERROR("%s: reg buf failed at idx(%d)", __func__, i);
            break;
        }

        /* start stream */
        rc = mm_stream_fsm_fn(s_objs[i],
                              MM_STREAM_EVT_START,
                              NULL,
                              NULL);
        if (0 != rc) {
            CDBG_ERROR("%s: start stream failed at idx(%d)", __func__, i);
            break;
        }
    }

    /* error handling */
    if (0 != rc) {
        for (j=0; j<=i; j++) {
            /* stop streams*/
            mm_stream_fsm_fn(s_objs[j],
                             MM_STREAM_EVT_STOP,
                             NULL,
                             NULL);

            /* unreg buf */
            mm_stream_fsm_fn(s_objs[j],
                             MM_STREAM_EVT_UNREG_BUF,
                             NULL,
                             NULL);

            /* put buf back */
            mm_stream_fsm_fn(s_objs[j],
                             MM_STREAM_EVT_PUT_BUF,
                             NULL,
                             NULL);
        }
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_stop
 *
 * DESCRIPTION: stop a channel, which will stop all streams in the channel
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_stop(mm_channel_t *my_obj)
{
    int32_t rc = 0;
    int i;
    mm_stream_t *s_objs[MAX_STREAM_NUM_IN_BUNDLE] = {NULL};
    uint8_t num_streams_to_stop = 0;
    mm_stream_t *s_obj = NULL;
    int meta_stream_idx = 0;

    for (i = 0; i < MAX_STREAM_NUM_IN_BUNDLE; i++) {
        if (my_obj->streams[i].my_hdl > 0) {
            s_obj = mm_channel_util_get_stream_by_handler(my_obj,
                                                          my_obj->streams[i].my_hdl);
            if (NULL != s_obj) {
                /* remember meta data stream index */
                if (s_obj->stream_info->stream_type == CAM_STREAM_TYPE_METADATA) {
                    meta_stream_idx = num_streams_to_stop;
                }
                s_objs[num_streams_to_stop++] = s_obj;
            }
        }
    }

    if (meta_stream_idx < num_streams_to_stop - 1 ) {
        /* always stop meta data stream last, so switch the stream object with the last one */
        s_obj = s_objs[num_streams_to_stop - 1];
        s_objs[num_streams_to_stop - 1] = s_objs[meta_stream_idx];
        s_objs[meta_stream_idx] = s_obj;
    }

    for (i = 0; i < num_streams_to_stop; i++) {
        /* stream off */
        mm_stream_fsm_fn(s_objs[i],
                         MM_STREAM_EVT_STOP,
                         NULL,
                         NULL);

        /* unreg buf at kernel */
        mm_stream_fsm_fn(s_objs[i],
                         MM_STREAM_EVT_UNREG_BUF,
                         NULL,
                         NULL);
    }

    /* destroy super buf cmd thread */
    if (TRUE == my_obj->bundle.is_active) {
        /* first stop bundle thread */
        mm_camera_cmd_thread_release(&my_obj->cmd_thread);
        mm_camera_cmd_thread_release(&my_obj->cb_thread);

        /* deinit superbuf queue */
        mm_channel_superbuf_queue_deinit(&my_obj->bundle.superbuf_queue);

        /* memset bundle info */
        memset(&my_obj->bundle, 0, sizeof(mm_channel_bundle_t));
    }

    /* since all streams are stopped, we are safe to
     * release all buffers allocated in stream */
    for (i = 0; i < num_streams_to_stop; i++) {
        /* put buf back */
        mm_stream_fsm_fn(s_objs[i],
                         MM_STREAM_EVT_PUT_BUF,
                         NULL,
                         NULL);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_request_super_buf
 *
 * DESCRIPTION: for burst mode in bundle, reuqest certain amount of matched
 *              frames from superbuf queue
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *   @num_buf_requested : number of matched frames needed
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_request_super_buf(mm_channel_t *my_obj, uint32_t num_buf_requested)
{
    int32_t rc = 0;
    mm_camera_cmdcb_t* node = NULL;

    /* set pending_cnt
     * will trigger dispatching super frames if pending_cnt > 0 */
    /* send cam_sem_post to wake up cmd thread to dispatch super buffer */
    node = (mm_camera_cmdcb_t *)malloc(sizeof(mm_camera_cmdcb_t));
    if (NULL != node) {
        memset(node, 0, sizeof(mm_camera_cmdcb_t));
        node->cmd_type = MM_CAMERA_CMD_TYPE_REQ_DATA_CB;
        node->u.req_buf.num_buf_requested = num_buf_requested;

        /* enqueue to cmd thread */
        cam_queue_enq(&(my_obj->cmd_thread.cmd_queue), node);

        /* wake up cmd thread */
        cam_sem_post(&(my_obj->cmd_thread.cmd_sem));
    } else {
        CDBG_ERROR("%s: No memory for mm_camera_node_t", __func__);
        rc = -1;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_cancel_super_buf_request
 *
 * DESCRIPTION: for burst mode in bundle, cancel the reuqest for certain amount
 *              of matched frames from superbuf queue
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_cancel_super_buf_request(mm_channel_t *my_obj)
{
    int32_t rc = 0;
    /* reset pending_cnt */
    rc = mm_channel_request_super_buf(my_obj, 0);
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_flush_super_buf_queue
 *
 * DESCRIPTION: flush superbuf queue
 *
 * PARAMETERS :
 *   @my_obj  : channel object
 *   @frame_idx : frame idx until which to flush all superbufs
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_flush_super_buf_queue(mm_channel_t *my_obj, uint32_t frame_idx)
{
    int32_t rc = 0;
    mm_camera_cmdcb_t* node = NULL;

    node = (mm_camera_cmdcb_t *)malloc(sizeof(mm_camera_cmdcb_t));
    if (NULL != node) {
        memset(node, 0, sizeof(mm_camera_cmdcb_t));
        node->cmd_type = MM_CAMERA_CMD_TYPE_FLUSH_QUEUE;
        node->u.frame_idx = frame_idx;

        /* enqueue to cmd thread */
        cam_queue_enq(&(my_obj->cmd_thread.cmd_queue), node);

        /* wake up cmd thread */
        cam_sem_post(&(my_obj->cmd_thread.cmd_sem));
    } else {
        CDBG_ERROR("%s: No memory for mm_camera_node_t", __func__);
        rc = -1;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_config_notify_mode
 *
 * DESCRIPTION: configure notification mode
 *
 * PARAMETERS :
 *   @my_obj  : channel object
 *   @notify_mode : notification mode
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_config_notify_mode(mm_channel_t *my_obj,
                                      mm_camera_super_buf_notify_mode_t notify_mode)
{
    int32_t rc = 0;
    mm_camera_cmdcb_t* node = NULL;

    node = (mm_camera_cmdcb_t *)malloc(sizeof(mm_camera_cmdcb_t));
    if (NULL != node) {
        memset(node, 0, sizeof(mm_camera_cmdcb_t));
        node->u.notify_mode = notify_mode;
        node->cmd_type = MM_CAMERA_CMD_TYPE_CONFIG_NOTIFY;

        /* enqueue to cmd thread */
        cam_queue_enq(&(my_obj->cmd_thread.cmd_queue), node);

        /* wake up cmd thread */
        cam_sem_post(&(my_obj->cmd_thread.cmd_sem));
    } else {
        CDBG_ERROR("%s: No memory for mm_camera_node_t", __func__);
        rc = -1;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_qbuf
 *
 * DESCRIPTION: enqueue buffer back to kernel
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *   @buf          : buf ptr to be enqueued
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_qbuf(mm_channel_t *my_obj,
                        mm_camera_buf_def_t *buf)
{
    int32_t rc = -1;
    mm_stream_t* s_obj = mm_channel_util_get_stream_by_handler(my_obj, buf->stream_id);

    if (NULL != s_obj) {
        rc = mm_stream_fsm_fn(s_obj,
                              MM_STREAM_EVT_QBUF,
                              (void *)buf,
                              NULL);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_set_stream_parms
 *
 * DESCRIPTION: set parameters per stream
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *   @s_id         : stream handle
 *   @parms        : ptr to a param struct to be set to server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : Assume the parms struct buf is already mapped to server via
 *              domain socket. Corresponding fields of parameters to be set
 *              are already filled in by upper layer caller.
 *==========================================================================*/
int32_t mm_channel_set_stream_parm(mm_channel_t *my_obj,
                                   mm_evt_paylod_set_get_stream_parms_t *payload)
{
    int32_t rc = -1;
    mm_stream_t* s_obj = mm_channel_util_get_stream_by_handler(my_obj,
                                                               payload->stream_id);
    if (NULL != s_obj) {
        rc = mm_stream_fsm_fn(s_obj,
                              MM_STREAM_EVT_SET_PARM,
                              (void *)payload,
                              NULL);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_get_stream_parms
 *
 * DESCRIPTION: get parameters per stream
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *   @s_id         : stream handle
 *   @parms        : ptr to a param struct to be get from server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : Assume the parms struct buf is already mapped to server via
 *              domain socket. Parameters to be get from server are already
 *              filled in by upper layer caller. After this call, corresponding
 *              fields of requested parameters will be filled in by server with
 *              detailed information.
 *==========================================================================*/
int32_t mm_channel_get_stream_parm(mm_channel_t *my_obj,
                                   mm_evt_paylod_set_get_stream_parms_t *payload)
{
    int32_t rc = -1;
    mm_stream_t* s_obj = mm_channel_util_get_stream_by_handler(my_obj,
                                                               payload->stream_id);
    if (NULL != s_obj) {
        rc = mm_stream_fsm_fn(s_obj,
                              MM_STREAM_EVT_GET_PARM,
                              (void *)payload,
                              NULL);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_do_stream_action
 *
 * DESCRIPTION: request server to perform stream based action. Maybe removed later
 *              if the functionality is included in mm_camera_set_parms
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *   @s_id         : stream handle
 *   @actions      : ptr to an action struct buf to be performed by server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : Assume the action struct buf is already mapped to server via
 *              domain socket. Actions to be performed by server are already
 *              filled in by upper layer caller.
 *==========================================================================*/
int32_t mm_channel_do_stream_action(mm_channel_t *my_obj,
                                   mm_evt_paylod_do_stream_action_t *payload)
{
    int32_t rc = -1;
    mm_stream_t* s_obj = mm_channel_util_get_stream_by_handler(my_obj,
                                                               payload->stream_id);
    if (NULL != s_obj) {
        rc = mm_stream_fsm_fn(s_obj,
                              MM_STREAM_EVT_DO_ACTION,
                              (void *)payload,
                              NULL);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_map_stream_buf
 *
 * DESCRIPTION: mapping stream buffer via domain socket to server
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *   @payload      : ptr to payload for mapping
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_map_stream_buf(mm_channel_t *my_obj,
                                  mm_evt_paylod_map_stream_buf_t *payload)
{
    int32_t rc = -1;
    mm_stream_t* s_obj = mm_channel_util_get_stream_by_handler(my_obj,
                                                               payload->stream_id);
    if (NULL != s_obj) {
        rc = mm_stream_map_buf(s_obj,
                               payload->buf_type,
                               payload->buf_idx,
                               payload->plane_idx,
                               payload->fd,
                               payload->size);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_unmap_stream_buf
 *
 * DESCRIPTION: unmapping stream buffer via domain socket to server
 *
 * PARAMETERS :
 *   @my_obj       : channel object
 *   @payload      : ptr to unmap payload
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_unmap_stream_buf(mm_channel_t *my_obj,
                                    mm_evt_paylod_unmap_stream_buf_t *payload)
{
    int32_t rc = -1;
    mm_stream_t* s_obj = mm_channel_util_get_stream_by_handler(my_obj,
                                                               payload->stream_id);
    if (NULL != s_obj) {
        rc = mm_stream_unmap_buf(s_obj, payload->buf_type,
                                 payload->buf_idx, payload->plane_idx);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_superbuf_queue_init
 *
 * DESCRIPTION: initialize superbuf queue in the channel
 *
 * PARAMETERS :
 *   @queue   : ptr to superbuf queue to be initialized
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_superbuf_queue_init(mm_channel_queue_t * queue)
{
    return cam_queue_init(&queue->que);
}

/*===========================================================================
 * FUNCTION   : mm_channel_superbuf_queue_deinit
 *
 * DESCRIPTION: deinitialize superbuf queue in the channel
 *
 * PARAMETERS :
 *   @queue   : ptr to superbuf queue to be deinitialized
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_superbuf_queue_deinit(mm_channel_queue_t * queue)
{
    return cam_queue_deinit(&queue->que);
}

/*===========================================================================
 * FUNCTION   : mm_channel_util_seq_comp_w_rollover
 *
 * DESCRIPTION: utility function to handle sequence number comparison with rollover
 *
 * PARAMETERS :
 *   @v1      : first value to be compared
 *   @v2      : second value to be compared
 *
 * RETURN     : int8_t type of comparison result
 *              >0  -- v1 larger than v2
 *              =0  -- vi equal to v2
 *              <0  -- v1 smaller than v2
 *==========================================================================*/
int8_t mm_channel_util_seq_comp_w_rollover(uint32_t v1,
                                           uint32_t v2)
{
    int8_t ret = 0;

    /* TODO: need to handle the case if v2 roll over to 0 */
    if (v1 > v2) {
        ret = 1;
    } else if (v1 < v2) {
        ret = -1;
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   : mm_channel_handle_metadata
 *
 * DESCRIPTION: Handle frame matching logic change due to metadata
 *
 * PARAMETERS :
 *   @ch_obj  : channel object
 *   @queue   : superbuf queue
 *   @buf_info: new buffer from stream
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_handle_metadata(
                        mm_channel_t* ch_obj,
                        mm_channel_queue_t * queue,
                        mm_camera_buf_info_t *buf_info)
{
    int rc = 0 ;
    mm_stream_t* stream_obj = NULL;
    stream_obj = mm_channel_util_get_stream_by_handler(ch_obj,
                buf_info->stream_id);

    if (NULL == stream_obj) {
        CDBG_ERROR("%s: Invalid Stream Object for stream_id = %d",
                   __func__, buf_info->stream_id);
        rc = -1;
        goto end;
    }
    if (NULL == stream_obj->stream_info) {
        CDBG_ERROR("%s: NULL stream info for stream_id = %d",
                    __func__, buf_info->stream_id);
        rc = -1;
        goto end;
    }

    if (CAM_STREAM_TYPE_METADATA == stream_obj->stream_info->stream_type) {
        const cam_metadata_info_t *metadata;
        metadata = (const cam_metadata_info_t *)buf_info->buf->buffer;

        if (NULL == metadata) {
            CDBG_ERROR("%s: NULL metadata buffer for metadata stream",
                       __func__);
            rc = -1;
            goto end;
        }

        if (metadata->is_prep_snapshot_done_valid &&
                metadata->is_good_frame_idx_range_valid) {
            CDBG_ERROR("%s: prep_snapshot_done and good_idx_range shouldn't be valid at the same time", __func__);
            rc = -1;
            goto end;
        }

        if (metadata->is_prep_snapshot_done_valid &&
            metadata->prep_snapshot_done_state == NEED_FUTURE_FRAME) {

            /* Set expected frame id to a future frame idx, large enough to wait
             * for good_frame_idx_range, and small enough to still capture an image */
            const int max_future_frame_offset = 100;
            queue->expected_frame_id += max_future_frame_offset;

            mm_channel_superbuf_flush(ch_obj, queue);
        } else if (metadata->is_good_frame_idx_range_valid) {
            if (metadata->good_frame_idx_range.min_frame_idx >
                queue->expected_frame_id) {
                CDBG_HIGH("%s: min_frame_idx %d is greater than expected_frame_id %d",
                    __func__, metadata->good_frame_idx_range.min_frame_idx,
                    queue->expected_frame_id);
            }
            queue->expected_frame_id =
                metadata->good_frame_idx_range.min_frame_idx;
        }
    }
end:
    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_superbuf_comp_and_enqueue
 *
 * DESCRIPTION: implementation for matching logic for superbuf
 *
 * PARAMETERS :
 *   @ch_obj  : channel object
 *   @queue   : superbuf queue
 *   @buf_info: new buffer from stream
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_superbuf_comp_and_enqueue(
                        mm_channel_t* ch_obj,
                        mm_channel_queue_t *queue,
                        mm_camera_buf_info_t *buf_info)
{
    cam_node_t* node = NULL;
    struct cam_list *head = NULL;
    struct cam_list *pos = NULL;
    mm_channel_queue_node_t* super_buf = NULL;
    uint8_t buf_s_idx, i, found_super_buf, unmatched_bundles;
    struct cam_list *last_buf, *insert_before_buf;

    CDBG("%s: E", __func__);
    for (buf_s_idx = 0; buf_s_idx < queue->num_streams; buf_s_idx++) {
        if (buf_info->stream_id == queue->bundled_streams[buf_s_idx]) {
            break;
        }
    }
    if (buf_s_idx == queue->num_streams) {
        CDBG_ERROR("%s: buf from stream (%d) not bundled", __func__, buf_info->stream_id);
        return -1;
    }

    if (mm_channel_handle_metadata(ch_obj, queue, buf_info) < 0) {
        return -1;
    }

    if (mm_channel_util_seq_comp_w_rollover(buf_info->frame_idx,
                                            queue->expected_frame_id) < 0) {
        /* incoming buf is older than expected buf id, will discard it */
        mm_channel_qbuf(ch_obj, buf_info->buf);
        return 0;
    }

    if (MM_CAMERA_SUPER_BUF_PRIORITY_NORMAL != queue->attr.priority) {
        /* TODO */
        /* need to decide if we want to queue the frame based on focus or exposure
         * if frame not to be queued, we need to qbuf it back */
    }

    /* comp */
    pthread_mutex_lock(&queue->que.lock);
    head = &queue->que.head.list;
    /* get the last one in the queue which is possibly having no matching */
    pos = head->next;

    found_super_buf = 0;
    unmatched_bundles = 0;
    last_buf = NULL;
    insert_before_buf = NULL;
    while (pos != head) {
        node = member_of(pos, cam_node_t, list);
        super_buf = (mm_channel_queue_node_t*)node->data;
        if (NULL != super_buf) {
            if (super_buf->matched) {
                /* find a matched super buf, move to next one */
                pos = pos->next;
                continue;
            } else if ( buf_info->frame_idx == super_buf->frame_idx ) {
                /* have an unmatched super buf that matches our frame idx,
                 *  break the loop */
                found_super_buf = 1;
                break;
            } else {
                unmatched_bundles++;
                if ( NULL == last_buf ) {
                    if ( super_buf->frame_idx < buf_info->frame_idx ) {
                        last_buf = pos;
                    }
                }
                if ( NULL == insert_before_buf ) {
                    if ( super_buf->frame_idx > buf_info->frame_idx ) {
                        insert_before_buf = pos;
                    }
                }
                pos = pos->next;
            }
        }
    }

    if ( found_super_buf ) {
            super_buf->super_buf[buf_s_idx] = *buf_info;

            /* check if superbuf is all matched */
            super_buf->matched = 1;
            for (i=0; i < super_buf->num_of_bufs; i++) {
                if (super_buf->super_buf[i].frame_idx == 0) {
                    super_buf->matched = 0;
                    break;
                }
            }

            if (super_buf->matched) {
                queue->expected_frame_id = buf_info->frame_idx + queue->attr.post_frame_skip;
                queue->match_cnt++;
                /* Any older unmatched buffer need to be released */
                if ( last_buf ) {
                    while ( last_buf != pos ) {
                        node = member_of(last_buf, cam_node_t, list);
                        super_buf = (mm_channel_queue_node_t*)node->data;
                        if (NULL != super_buf) {
                            for (i=0; i<super_buf->num_of_bufs; i++) {
                                if (super_buf->super_buf[i].frame_idx != 0) {
                                        mm_channel_qbuf(ch_obj, super_buf->super_buf[i].buf);
                                }
                            }
                            queue->que.size--;
                            last_buf = last_buf->next;
                            cam_list_del_node(&node->list);
                            free(node);
                            free(super_buf);
                        } else {
                            CDBG_ERROR(" %s : Invalid superbuf in queue!", __func__);
                            break;
                        }
                    }
                }
            }
    } else {
        if (  ( queue->attr.max_unmatched_frames < unmatched_bundles ) &&
              ( NULL == last_buf ) ) {
            /* incoming frame is older than the last bundled one */
            mm_channel_qbuf(ch_obj, buf_info->buf);
        } else {
            if ( queue->attr.max_unmatched_frames < unmatched_bundles ) {
                /* release the oldest bundled superbuf */
                node = member_of(last_buf, cam_node_t, list);
                super_buf = (mm_channel_queue_node_t*)node->data;
                for (i=0; i<super_buf->num_of_bufs; i++) {
                    if (super_buf->super_buf[i].frame_idx != 0) {
                            mm_channel_qbuf(ch_obj, super_buf->super_buf[i].buf);
                    }
                }
                queue->que.size--;
                node = member_of(last_buf, cam_node_t, list);
                cam_list_del_node(&node->list);
                free(node);
                free(super_buf);
            }
            /* insert the new frame at the appropriate position. */

            mm_channel_queue_node_t *new_buf = NULL;
            cam_node_t* new_node = NULL;

            new_buf = (mm_channel_queue_node_t*)malloc(sizeof(mm_channel_queue_node_t));
            new_node = (cam_node_t*)malloc(sizeof(cam_node_t));
            if (NULL != new_buf && NULL != new_node) {
                memset(new_buf, 0, sizeof(mm_channel_queue_node_t));
                memset(new_node, 0, sizeof(cam_node_t));
                new_node->data = (void *)new_buf;
                new_buf->num_of_bufs = queue->num_streams;
                new_buf->super_buf[buf_s_idx] = *buf_info;
                new_buf->frame_idx = buf_info->frame_idx;

                /* enqueue */
                if ( insert_before_buf ) {
                    cam_list_insert_before_node(&new_node->list, insert_before_buf);
                } else {
                    cam_list_add_tail_node(&new_node->list, &queue->que.head.list);
                }
                queue->que.size++;

                if(queue->num_streams == 1) {
                    new_buf->matched = 1;

                    queue->expected_frame_id = buf_info->frame_idx + queue->attr.post_frame_skip;
                    queue->match_cnt++;
                }
            } else {
                /* No memory */
                if (NULL != new_buf) {
                    free(new_buf);
                }
                if (NULL != new_node) {
                    free(new_node);
                }
                /* qbuf the new buf since we cannot enqueue */
                mm_channel_qbuf(ch_obj, buf_info->buf);
            }
        }
    }

    pthread_mutex_unlock(&queue->que.lock);

    CDBG("%s: X", __func__);
    return 0;
}

/*===========================================================================
 * FUNCTION   : mm_channel_superbuf_dequeue_internal
 *
 * DESCRIPTION: internal implementation for dequeue from the superbuf queue
 *
 * PARAMETERS :
 *   @queue   : superbuf queue
 *   @matched_only : if dequeued buf should be matched
 *
 * RETURN     : ptr to a node from superbuf queue
 *==========================================================================*/
mm_channel_queue_node_t* mm_channel_superbuf_dequeue_internal(mm_channel_queue_t * queue,
                                                              uint8_t matched_only)
{
    cam_node_t* node = NULL;
    struct cam_list *head = NULL;
    struct cam_list *pos = NULL;
    mm_channel_queue_node_t* super_buf = NULL;

    head = &queue->que.head.list;
    pos = head->next;
    if (pos != head) {
        /* get the first node */
        node = member_of(pos, cam_node_t, list);
        super_buf = (mm_channel_queue_node_t*)node->data;
        if ( (NULL != super_buf) &&
             (matched_only == TRUE) &&
             (super_buf->matched == FALSE) ) {
            /* require to dequeue matched frame only, but this superbuf is not matched,
               simply set return ptr to NULL */
            super_buf = NULL;
        }
        if (NULL != super_buf) {
            /* remove from the queue */
            cam_list_del_node(&node->list);
            queue->que.size--;
            if (super_buf->matched == TRUE) {
                queue->match_cnt--;
            }
            free(node);
        }
    }

    return super_buf;
}

/*===========================================================================
 * FUNCTION   : mm_channel_superbuf_dequeue
 *
 * DESCRIPTION: dequeue from the superbuf queue
 *
 * PARAMETERS :
 *   @queue   : superbuf queue
 *
 * RETURN     : ptr to a node from superbuf queue
 *==========================================================================*/
mm_channel_queue_node_t* mm_channel_superbuf_dequeue(mm_channel_queue_t * queue)
{
    mm_channel_queue_node_t* super_buf = NULL;

    pthread_mutex_lock(&queue->que.lock);
    super_buf = mm_channel_superbuf_dequeue_internal(queue, TRUE);
    pthread_mutex_unlock(&queue->que.lock);

    return super_buf;
}

/*===========================================================================
 * FUNCTION   : mm_channel_superbuf_bufdone_overflow
 *
 * DESCRIPTION: keep superbuf queue no larger than watermark set by upper layer
 *              via channel attribute
 *
 * PARAMETERS :
 *   @my_obj  : channel object
 *   @queue   : superbuf queue
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_superbuf_bufdone_overflow(mm_channel_t* my_obj,
                                             mm_channel_queue_t * queue)
{
    int32_t rc = 0, i;
    mm_channel_queue_node_t* super_buf = NULL;
    if (MM_CAMERA_SUPER_BUF_NOTIFY_CONTINUOUS == queue->attr.notify_mode) {
        /* for continuous streaming mode, no overflow is needed */
        return 0;
    }

    CDBG("%s: before match_cnt=%d, water_mark=%d",
         __func__, queue->match_cnt, queue->attr.water_mark);
    /* bufdone overflowed bufs */
    pthread_mutex_lock(&queue->que.lock);
    while (queue->match_cnt > queue->attr.water_mark) {
        super_buf = mm_channel_superbuf_dequeue_internal(queue, TRUE);
        if (NULL != super_buf) {
            for (i=0; i<super_buf->num_of_bufs; i++) {
                if (NULL != super_buf->super_buf[i].buf) {
                    mm_channel_qbuf(my_obj, super_buf->super_buf[i].buf);
                }
            }
            free(super_buf);
        }
    }
    pthread_mutex_unlock(&queue->que.lock);
    CDBG("%s: after match_cnt=%d, water_mark=%d",
         __func__, queue->match_cnt, queue->attr.water_mark);

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_superbuf_skip
 *
 * DESCRIPTION: depends on the lookback configuration of the channel attribute,
 *              unwanted superbufs will be removed from the superbuf queue.
 *
 * PARAMETERS :
 *   @my_obj  : channel object
 *   @queue   : superbuf queue
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_superbuf_skip(mm_channel_t* my_obj,
                                 mm_channel_queue_t * queue)
{
    int32_t rc = 0, i;
    mm_channel_queue_node_t* super_buf = NULL;
    if (MM_CAMERA_SUPER_BUF_NOTIFY_CONTINUOUS == queue->attr.notify_mode) {
        /* for continuous streaming mode, no skip is needed */
        return 0;
    }

    /* bufdone overflowed bufs */
    pthread_mutex_lock(&queue->que.lock);
    while (queue->match_cnt > queue->attr.look_back) {
        super_buf = mm_channel_superbuf_dequeue_internal(queue, TRUE);
        if (NULL != super_buf) {
            for (i=0; i<super_buf->num_of_bufs; i++) {
                if (NULL != super_buf->super_buf[i].buf) {
                    mm_channel_qbuf(my_obj, super_buf->super_buf[i].buf);
                }
            }
            free(super_buf);
        }
    }
    pthread_mutex_unlock(&queue->que.lock);

    return rc;
}

/*===========================================================================
 * FUNCTION   : mm_channel_superbuf_flush
 *
 * DESCRIPTION: flush the superbuf queue.
 *
 * PARAMETERS :
 *   @my_obj  : channel object
 *   @queue   : superbuf queue
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *==========================================================================*/
int32_t mm_channel_superbuf_flush(mm_channel_t* my_obj,
                                  mm_channel_queue_t * queue)
{
    int32_t rc = 0, i;
    mm_channel_queue_node_t* super_buf = NULL;

    /* bufdone bufs */
    pthread_mutex_lock(&queue->que.lock);
    super_buf = mm_channel_superbuf_dequeue_internal(queue, FALSE);
    while (super_buf != NULL) {
        for (i=0; i<super_buf->num_of_bufs; i++) {
            if (NULL != super_buf->super_buf[i].buf) {
                mm_channel_qbuf(my_obj, super_buf->super_buf[i].buf);
            }
        }
        free(super_buf);
        super_buf = mm_channel_superbuf_dequeue_internal(queue, FALSE);
    }
    pthread_mutex_unlock(&queue->que.lock);

    return rc;
}
