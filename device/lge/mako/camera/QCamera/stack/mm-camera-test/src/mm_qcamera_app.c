/*
Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
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
*/

#include <pthread.h>
#include "mm_camera_dbg.h"
#include <errno.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <dlfcn.h>
#include "mm_qcamera_main_menu.h"
#include "mm_qcamera_app.h"


mm_camera_app_t my_cam_app;

static pthread_mutex_t app_mutex;
static int thread_status = 0;
static pthread_cond_t app_cond_v;

#define MM_QCAMERA_APP_WAIT_TIME 1000000000

int mm_camera_app_timedwait()
{
    int rc = 0;
    pthread_mutex_lock(&app_mutex);
    if(false == thread_status) {
        struct timespec tw;
        memset(&tw, 0, sizeof tw);
        tw.tv_sec = 0;
        tw.tv_nsec = time(0) + MM_QCAMERA_APP_WAIT_TIME;

        //pthread_cond_wait(&app_cond_v, &app_mutex);
        rc = pthread_cond_timedwait(&app_cond_v, &app_mutex,&tw);
        thread_status = false;
    }
    pthread_mutex_unlock(&app_mutex);
    return rc;
}

int mm_camera_app_wait()
{
    int rc = 0;
    pthread_mutex_lock(&app_mutex);
    if(false == thread_status){
        pthread_cond_wait(&app_cond_v, &app_mutex);
        thread_status = false;
    }
    pthread_mutex_unlock(&app_mutex);
    return rc;
}

void mm_camera_app_done()
{
  pthread_mutex_lock(&app_mutex);
  thread_status = true;
  pthread_cond_signal(&app_cond_v);
  pthread_mutex_unlock(&app_mutex);
}


mm_camera_app_obj_t *mm_app_get_cam_obj(int8_t cam_id)
{
    mm_camera_app_obj_t *temp = my_cam_app.obj[cam_id];
    return temp;
}

void mm_app_user_ptr(int use_user_ptr)
{
    my_cam_app.use_user_ptr = use_user_ptr;
}

void mm_app_set_dim_def(cam_ctrl_dimension_t *dim)
{
    dim->display_width = WVGA_WIDTH;
    dim->display_height = WVGA_HEIGHT;
    input_display.user_input_display_width = dim->display_width;
    input_display.user_input_display_height = dim->display_height;
    dim->video_width = WVGA_WIDTH;
    dim->video_width = CEILING32(dim->video_width);
    dim->video_height = WVGA_HEIGHT;
    dim->orig_video_width = dim->video_width;
    dim->orig_video_height = dim->video_height;
    dim->picture_width = MP1_WIDTH;
    dim->picture_height = MP1_HEIGHT;
    dim->orig_picture_dx = dim->picture_width;
    dim->orig_picture_dy = dim->picture_height;
    dim->ui_thumbnail_height = QVGA_HEIGHT;
    dim->ui_thumbnail_width = QVGA_WIDTH;
    dim->thumbnail_height = dim->ui_thumbnail_height;
    dim->thumbnail_width = dim->ui_thumbnail_width;
    dim->orig_picture_width = dim->picture_width;
    dim->orig_picture_height = dim->picture_height;
    dim->orig_thumb_width = dim->thumbnail_width;
    dim->orig_thumb_height = dim->thumbnail_height;
    dim->raw_picture_height = MP1_HEIGHT;
    dim->raw_picture_width = MP1_WIDTH;
    dim->hjr_xtra_buff_for_bayer_filtering;
    dim->prev_format = CAMERA_YUV_420_NV21;
    dim->enc_format = CAMERA_YUV_420_NV12;
    dim->thumb_format = CAMERA_YUV_420_NV21;
    dim->main_img_format = CAMERA_YUV_420_NV21;
    dim->prev_padding_format = CAMERA_PAD_TO_4K;
    dim->display_luma_width = dim->display_width;
    dim->display_luma_height = dim->display_height;
    dim->display_chroma_width = dim->display_width;
    dim->display_chroma_height = dim->display_height;
    dim->video_luma_width = dim->orig_video_width;
    dim->video_luma_height = dim->orig_video_height;
    dim->video_chroma_width = dim->orig_video_width;
    dim->video_chroma_height = dim->orig_video_height;
    dim->thumbnail_luma_width = dim->thumbnail_width;
    dim->thumbnail_luma_height = dim->thumbnail_height;
    dim->thumbnail_chroma_width = dim->thumbnail_width;
    dim->thumbnail_chroma_height = dim->thumbnail_height;
    dim->main_img_luma_width = dim->picture_width;
    dim->main_img_luma_height = dim->picture_height;
    dim->main_img_chroma_width = dim->picture_width;
    dim->main_img_chroma_height = dim->picture_height;
}

int mm_app_load_hal()
{
  memset(&my_cam_app, 0, sizeof(my_cam_app));
  memset(&my_cam_app.hal_lib, 0, sizeof(hal_interface_lib_t));
#if defined(_MSM7630_)
   my_cam_app.hal_lib.ptr = dlopen("/usr/lib/hw/camera.msm7630.so", RTLD_LAZY);
#elif defined(_MSM7627A_)
   my_cam_app.hal_lib.ptr = dlopen("/usr/lib/hw/camera.msm7627A.so", RTLD_LAZY);
#else
  //my_cam_app.hal_lib.ptr = dlopen("hw/camera.msm8960.so", RTLD_NOW);
   my_cam_app.hal_lib.ptr = dlopen("libmmcamera_interface_badger.so", RTLD_NOW);
#endif
  if (!my_cam_app.hal_lib.ptr) {
    CDBG_ERROR("%s Error opening HAL library %s\n", __func__, dlerror());
    return -1;
  }
  *(void **)&(my_cam_app.hal_lib.mm_camera_query) =
        dlsym(my_cam_app.hal_lib.ptr,
              "camera_query");
  *(void **)&(my_cam_app.hal_lib.mm_camera_open) =
        dlsym(my_cam_app.hal_lib.ptr,
              "camera_open");
  return 0;
}

int mm_app_init()
{
    int rc = MM_CAMERA_OK;
    CDBG("%s:BEGIN\n", __func__);
    my_cam_app.cam_info =  (mm_camera_info_t *)my_cam_app.hal_lib.mm_camera_query(&my_cam_app.num_cameras);
    if(my_cam_app.cam_info == NULL) {
        CDBG_ERROR("%s: Failed to query camera\n", __func__);
        rc = -1;
    }
    CDBG("%s:END, num_cameras = %d\n", __func__, my_cam_app.num_cameras);
    return rc;
}

static void notify_evt_cb(uint32_t camera_handle,
                         mm_camera_event_t *evt,void *user_data)
{
    CDBG("%s:E evt = %d",__func__,evt->event_type);

    switch(evt->event_type)
    {
        case MM_CAMERA_EVT_TYPE_CH:
        break;
        case MM_CAMERA_EVT_TYPE_CTRL:
            break;
        case MM_CAMERA_EVT_TYPE_STATS:
            break;
        case MM_CAMERA_EVT_TYPE_INFO:
        break;
        default:
            break;
    }
    CDBG("%s:X",__func__);
}

int mm_app_open(uint8_t cam_id)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = NULL;
    int i;
    mm_camera_event_type_t evt;

    pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s:BEGIN\n", __func__);
    if(pme != NULL) {
        CDBG("%s:cam already open.nop\n",__func__);
        goto end;
    }
    my_cam_app.cam_open = cam_id;
    my_cam_app.obj[cam_id] = (mm_camera_app_obj_t *)malloc(sizeof(mm_camera_app_obj_t));
    pme = my_cam_app.obj[cam_id];

    pme->mem_cam = (mm_camear_mem_vtbl_t *)malloc(sizeof(mm_camear_mem_vtbl_t));
    memset(pme->mem_cam,0,sizeof(mm_camear_mem_vtbl_t));
    pme->mem_cam->user_data = pme;

    pme->cam = my_cam_app.hal_lib.mm_camera_open(cam_id,pme->mem_cam);
    if(pme->cam == NULL) {
        CDBG("%s:dev open error=%d\n", __func__, rc);
        memset(pme,0, sizeof(pme));
        return -1;
    }
    CDBG("Open Camera id = %d handle = %d",cam_id,pme->cam->camera_handle);

    pme->ch_id = cam_id;
    pme->open_flag = true;
    mm_app_set_dim_def(&pme->dim);

    for (i = 0; i < MM_CAMERA_EVT_TYPE_MAX; i++) {
        evt = (mm_camera_event_type_t) i;
        pme->cam->ops->register_event_notify(pme->cam->camera_handle,notify_evt_cb,pme,evt);
    }
    pme->cam_state = CAMERA_STATE_OPEN;
    pme->cam_mode = CAMERA_MODE;
    pme->fullSizeSnapshot = 0;

    pme->ch_id = pme->cam->ops->ch_acquire(pme->cam->camera_handle);
    CDBG("Channel Acquired Successfully %d",pme->ch_id);
end:
    CDBG("%s:END, rc=%d\n", __func__, rc); 
    return rc;
}

int mm_app_close(int8_t cam_id)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s:BEGIN\n", __func__);
    if(!pme->cam) {
        CDBG("%s:cam already closed. nop\n",__func__);
        goto end;
    }

    pme->cam->ops->ch_release(pme->cam->camera_handle,pme->ch_id);
    pme->cam->ops->camera_close(pme->cam->camera_handle);
    pme->open_flag = false;
    pme->cam = NULL;
    pme->my_id = 0;
    free(pme->mem_cam);
    pme->mem_cam = NULL;
    memset(&pme->dim, 0, sizeof(pme->dim));
    memset(&pme->dim, 0, sizeof(pme->dim));

    free(pme);
    pme = NULL;
    my_cam_app.obj[cam_id] = NULL;
    
end:
    CDBG("%s:END, rc=%d\n", __func__, rc);
    return rc;
}

void switchRes(int cam_id)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    switch(pme->cam_state) {
    case CAMERA_STATE_RECORD:
    if(MM_CAMERA_OK != stopRecording(cam_id)){
        CDBG_ERROR("%s:Cannot stop video err=%d\n", __func__, rc);
        return -1;
    }
    case CAMERA_STATE_PREVIEW:
        if(MM_CAMERA_OK !=  mm_app_stop_preview(cam_id)){
            CDBG_ERROR("%s: Cannot switch to camera mode=%d\n", __func__);
            return -1;
        }
        break;
    case CAMERA_STATE_SNAPSHOT:
    default:
        break;
    }
}
void switchCamera(int cam_id)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    if(my_cam_app.cam_open == cam_id){
        return;
    }

    switch(pme->cam_state) {
        case CAMERA_STATE_RECORD:
            if(MM_CAMERA_OK != stopRecording(cam_id)){
                CDBG_ERROR("%s:Cannot stop video err=%d\n", __func__, rc);
                return -1;
            }
        case CAMERA_STATE_PREVIEW:
            if(MM_CAMERA_OK !=  mm_app_stop_preview(cam_id)){
                CDBG_ERROR("%s: Cannot switch to camera mode=%d\n", __func__);
                return -1;
            }
            break;
        case CAMERA_STATE_SNAPSHOT:
        default:
            break;
    }

    mm_app_close(my_cam_app.cam_open);
    mm_app_open(cam_id);
}

int mm_app_set_dim(int8_t cam_id, cam_ctrl_dimension_t *dim)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s:BEGIN\n", __func__);

    memcpy(&pme->dim, dim, sizeof(cam_ctrl_dimension_t));
    if(MM_CAMERA_OK != (rc = pme->cam->ops->set_parm(
            pme->cam->camera_handle,MM_CAMERA_PARM_DIMENSION, &pme->dim)))
    {
        CDBG_ERROR("%s: set dimension err=%d\n", __func__, rc);
    }
    CDBG("%s:END, rc=%d\n", __func__, rc);
    return rc;
}

int mm_app_get_dim(int8_t cam_id, cam_ctrl_dimension_t *dim)
{
    int rc = MM_CAMERA_OK;
#if 0
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    CDBG("%s:BEGIN\n", __func__);
    if(pme->open_flag != true) {
        CDBG("%s: dev not open yet\n", __func__);
        rc = -MM_CAMERA_E_INVALID_OPERATION;
        goto end;
    }
    /* now we only use the upper portion. TBD: needs to be fixed later */
    //memcpy(&pme->dim, dim, sizeof(cam_ctrl_dimension_t));
    if(MM_CAMERA_OK != (rc = pme->cam->cfg->get_parm(pme->cam,
                                                    MM_CAMERA_PARM_DIMENSION, &pme->dim))) {
        CDBG("%s: set dimension err=%d\n", __func__, rc);
    }
    CDBG("%s: raw_w=%d,raw_h=%d\n",
            __func__, pme->dim.orig_picture_width, pme->dim.orig_picture_height);
    if(dim)
    memcpy(dim, &pme->dim, sizeof(cam_ctrl_dimension_t));

end:
    CDBG("%s:END, rc=%d\n", __func__, rc);
#endif
    return rc;
}

void mm_app_close_ch(int cam_id, int ch_type)
{
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    pme->cam->ops->ch_release(pme->cam->camera_handle,pme->ch_id);
    CDBG("%s:END,cam_id = %d, ch = %d\n", __func__, cam_id, ch_type);

}

int mm_app_unit_test()
{
    my_cam_app.run_sanity = 1;
    mm_app_unit_test_entry(&my_cam_app);
    return 0;
}

int mm_app_dual_test()
{
    my_cam_app.run_sanity = 1;
    mm_app_dual_test_entry(&my_cam_app);
    return 0;
}
