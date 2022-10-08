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
#include <stdbool.h>
#include "mm_camera_dbg.h"
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include "mm_qcamera_app.h"

#define BUFF_SIZE_128 128

//static mm_camera_ch_data_buf_t *mCurrentFrameEncoded;
static int JpegOffset = 0;
static int raw_snapshot_cnt = 0;
static int snapshot_cnt = 0;
static pthread_mutex_t g_s_mutex;
static int g_status = 0;
static pthread_cond_t g_s_cond_v;


static void mm_app_snapshot_done()
{
    pthread_mutex_lock(&g_s_mutex);
    g_status = true;
    pthread_cond_signal(&g_s_cond_v);
    pthread_mutex_unlock(&g_s_mutex);
}

static int mm_app_dump_snapshot_frame(struct msm_frame *frame,
                                      uint32_t len, int is_main, int is_raw)
{
    char bufp[BUFF_SIZE_128];
    int file_fdp;
    int rc = 0;

    if (is_raw) {
        snprintf(bufp, BUFF_SIZE_128, "/data/main_raw_%d.yuv", raw_snapshot_cnt);
    } else {
        if (is_main) {
            snprintf(bufp, BUFF_SIZE_128, "/data/main_%d.yuv", snapshot_cnt);
        } else {
            snprintf(bufp, BUFF_SIZE_128, "/data/thumb_%d.yuv", snapshot_cnt);
        }
    }

    file_fdp = open(bufp, O_RDWR | O_CREAT, 0777);

    if (file_fdp < 0) {
        CDBG("cannot open file %s\n", bufp);
        rc = -1;
        goto end;
    }
    CDBG("%s:dump snapshot frame to '%s'\n", __func__, bufp);
    write(file_fdp,
          (const void *)frame->buffer, len);
    close(file_fdp);
    end:
    return rc;
}


static void mm_app_dump_jpeg_frame(const void * data, uint32_t size, char* name, char* ext, int index)
{
    char buf[32];
    int file_fd;
    if ( data != NULL) {
        char * str;
        snprintf(buf, sizeof(buf), "/data/%s_%d.%s", name, index, ext);
        CDBG("%s size =%d", buf, size);
        file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        write(file_fd, data, size);
        close(file_fd);
    }
}

static int mm_app_set_thumbnail_fmt(int cam_id,mm_camera_image_fmt_t *fmt)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    fmt->meta_header = MM_CAMEAR_META_DATA_TYPE_DEF;
    fmt->fmt = pme->dim.thumb_format;
    fmt->width = pme->dim.thumbnail_width;
    fmt->height = pme->dim.thumbnail_height;
    CDBG("Thumbnail Dimension = %dX%d",fmt->width,fmt->height);
    return rc;
}

int mm_app_set_snapshot_fmt(int cam_id,mm_camera_image_fmt_t *fmt)
{

    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    fmt->meta_header = MM_CAMEAR_META_DATA_TYPE_DEF;
    fmt->fmt = pme->dim.main_img_format;
    fmt->width = pme->dim.picture_width;
    fmt->height = pme->dim.picture_height;
    CDBG("Snapshot Dimension = %dX%d",fmt->width,fmt->height);
    return rc;
}

int mm_app_set_live_snapshot_fmt(int cam_id,mm_camera_image_fmt_t *fmt)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    fmt->meta_header = MM_CAMEAR_META_DATA_TYPE_DEF;
    fmt->fmt = pme->dim.enc_format;
    fmt->width = pme->dim.video_width;
    fmt->height = pme->dim.video_height;
    CDBG("Livesnapshot Dimension = %dX%d",fmt->width,fmt->height);
    return rc;
}

int mm_app_set_raw_snapshot_fmt(int cam_id)
{
    int rc = MM_CAMERA_OK;
#if 0
    /* now we hard code format */
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    mm_camera_ch_image_fmt_parm_t fmt;

    CDBG("%s: BEGIN\n", __func__);
    memset(&fmt, 0, sizeof(mm_camera_ch_image_fmt_parm_t));
    fmt.ch_type = MM_CAMERA_CH_RAW;
    fmt.def.fmt = CAMERA_BAYER_SBGGR10;
    fmt.def.dim.width = pme->dim.raw_picture_width;
    fmt.def.dim.height = pme->dim.raw_picture_height;
    rc = pme->cam->cfg->set_parm(pme->cam, MM_CAMERA_PARM_CH_IMAGE_FMT, &fmt);
    if (rc != MM_CAMERA_OK) {
        CDBG("%s:set raw snapshot format err=%d\n", __func__, rc);
    }
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc); 
#endif 
    return rc;
}

int mm_app_prepare_raw_snapshot_buf(int cam_id)
{
    int rc = MM_CAMERA_OK;
#if 0
    int j;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    mm_camera_reg_buf_t reg_buf;
    uint32_t y_off, cbcr_off;
    uint8_t num_planes_main;
    uint32_t planes_main[VIDEO_MAX_PLANES];

    CDBG("%s: BEGIN, raw_w=%d, raw_h=%d\n",
         __func__, pme->dim.raw_picture_width, pme->dim.raw_picture_height);
    memset(&reg_buf,  0,  sizeof(reg_buf));
    reg_buf.def.buf.mp = malloc(sizeof(mm_camera_mp_buf_t));
    if (!reg_buf.def.buf.mp) {
        CDBG_ERROR("%s Error allocating memory for mplanar struct ", __func__);
        rc = -MM_CAMERA_E_NO_MEMORY; 
        goto end;
    }

    // setup main buffer
    memset(&pme->raw_snapshot_buf, 0, sizeof(pme->raw_snapshot_buf));
    pme->raw_snapshot_buf.num = 1;
    pme->raw_snapshot_buf.frame_len = 
    my_cam_app.hal_lib.mm_camera_get_msm_frame_len(CAMERA_BAYER_SBGGR10,
                                                   CAMERA_MODE_2D,
                                                   pme->dim.raw_picture_width,
                                                   pme->dim.raw_picture_height,
                                                   OUTPUT_TYPE_S,
                                                   &num_planes_main,
                                                   planes_main);
#ifdef USE_ION
    pme->raw_snapshot_buf.frame[0].ion_alloc.len = pme->raw_snapshot_buf.frame_len;
    pme->raw_snapshot_buf.frame[0].ion_alloc.flags = (0x1 << CAMERA_ION_HEAP_ID);
    pme->raw_snapshot_buf.frame[0].ion_alloc.align = 4096;
#endif
    pme->raw_snapshot_buf.frame[0].buffer = (unsigned long) my_cam_app.hal_lib.mm_camera_do_mmap(
                                                                                                pme->raw_snapshot_buf.frame_len, &pme->raw_snapshot_buf.frame[0].fd);


    if (!pme->raw_snapshot_buf.frame[0].buffer) {
        CDBG("%s:no mem for snapshot buf\n", __func__);
        rc = -MM_CAMERA_E_NO_MEMORY;
        goto end;
    }
    pme->raw_snapshot_buf.frame[0].path = OUTPUT_TYPE_S;
    pme->preview_buf.frame[0].y_off = 0;
    pme->raw_snapshot_buf.frame[0].cbcr_off = planes_main[0];

    /*setup registration buffer*/
    reg_buf.def.buf.mp[0].frame = pme->raw_snapshot_buf.frame[0];
    reg_buf.def.buf.mp[0].frame_offset = 0;
    reg_buf.def.buf.mp[0].num_planes = num_planes_main;

    reg_buf.def.buf.mp[0].planes[0].length = planes_main[0];
    reg_buf.def.buf.mp[0].planes[0].m.userptr = pme->raw_snapshot_buf.frame[0].fd;
    reg_buf.def.buf.mp[0].planes[0].data_offset = 0;
    reg_buf.def.buf.mp[0].planes[0].reserved[0] = reg_buf.def.buf.mp[0].frame_offset;
    for (j = 1; j < num_planes_main; j++) {
        reg_buf.def.buf.mp[0].planes[j].length = planes_main[j];
        reg_buf.def.buf.mp[0].planes[j].m.userptr = pme->raw_snapshot_buf.frame[0].fd;
        reg_buf.def.buf.mp[0].planes[j].data_offset = 0;
        reg_buf.def.buf.mp[0].planes[j].reserved[0] = reg_buf.def.buf.mp[0].planes[j-1].reserved[0] +
                                                      reg_buf.def.buf.mp[0].planes[j-1].length;
    }

    reg_buf.ch_type = MM_CAMERA_CH_RAW;
    reg_buf.def.num = pme->raw_snapshot_buf.num;
    rc = pme->cam->cfg->prepare_buf(pme->cam, &reg_buf);
    if (rc != MM_CAMERA_OK) {
        CDBG("%s:reg snapshot buf err=%d\n", __func__, rc);
        goto end;
    }
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc); 
#endif 
    return rc;
}

static int mm_app_unprepare_raw_snapshot_buf(int cam_id)
{
    int i, rc = MM_CAMERA_OK;
#if 0
    /* now we hard code format */
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s: BEGIN\n", __func__);
    rc = pme->cam->cfg->unprepare_buf(pme->cam, MM_CAMERA_CH_RAW);
    rc = my_cam_app.hal_lib.mm_camera_do_munmap(pme->raw_snapshot_buf.frame[0].fd,
                                                (void *)pme->raw_snapshot_buf.frame[0].buffer,
                                                pme->raw_snapshot_buf.frame_len);
    rc = my_cam_app.hal_lib.mm_camera_do_munmap(pme->jpeg_buf.frame[0].fd,
                                                (void *)pme->jpeg_buf.frame[0].buffer,
                                                pme->jpeg_buf.frame_len);
    /* zero out the buf stuct */
    memset(&pme->raw_snapshot_buf, 0, sizeof(pme->raw_snapshot_buf));
    memset(&pme->jpeg_buf, 0, sizeof(pme->jpeg_buf));

    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
#endif
    return rc;
}

#ifndef DISABLE_JPEG_ENCODING
/* Once we give frame for encoding, we get encoded jpeg image
   fragments by fragment. We'll need to store them in a buffer
   to form complete JPEG image */
static void snapshot_jpeg_fragment_cb(uint8_t *ptr,
                                      uint32_t size,
                                      void *user_data)
{
#if 0
    mm_camera_app_obj_t *pme = user_data;

    CDBG("%s: E",__func__);
    if (pme) {
        memcpy((uint8_t *)((uint32_t)pme->jpeg_buf.frame[0].buffer + JpegOffset), ptr, size);
        JpegOffset += size;
    }
    CDBG("%s: X",__func__);
#endif
}
#endif
/* This callback is received once the complete JPEG encoding is done */
static void snapshot_raw_cb(mm_camera_super_buf_t *bufs,
                            void *user_data)
{

    int rc;
    int i = 0;
    mm_camera_buf_def_t *main_frame = NULL;
    mm_camera_buf_def_t *thumb_frame = NULL;
    mm_camera_app_obj_t *pme = NULL;
    CDBG("%s: BEGIN\n", __func__); 

    pme = (mm_camera_app_obj_t *)user_data;

    CDBG("%s : total streams = %d",__func__,bufs->num_bufs);
    main_frame = bufs->bufs[0];
    thumb_frame = bufs->bufs[1];

    CDBG("mainframe frame_idx = %d fd = %d frame length = %d",main_frame->frame_idx,main_frame->fd,main_frame->frame_len);
    CDBG("thumnail frame_idx = %d fd = %d frame length = %d",thumb_frame->frame_idx,thumb_frame->fd,thumb_frame->frame_len);

    //dumpFrameToFile(main_frame->frame,pme->dim.picture_width,pme->dim.picture_height,"main", 1);
    //dumpFrameToFile(thumb_frame->frame,pme->dim.thumbnail_width,pme->dim.thumbnail_height,"thumb", 1);

    dumpFrameToFile(main_frame,pme->dim.picture_width,pme->dim.picture_height,"main", 1);
    dumpFrameToFile(thumb_frame,pme->dim.thumbnail_width,pme->dim.thumbnail_height,"thumb", 1);

    if (MM_CAMERA_OK != pme->cam->ops->qbuf(pme->cam->camera_handle,pme->ch_id,main_frame)) {
        CDBG_ERROR("%s: Failed in thumbnail Qbuf\n", __func__); 
    }
    if (MM_CAMERA_OK != pme->cam->ops->qbuf(pme->cam->camera_handle,pme->ch_id,thumb_frame)) {
        CDBG_ERROR("%s: Failed in thumbnail Qbuf\n", __func__); 
    }

    mm_app_snapshot_done();
    CDBG("%s: END\n", __func__); 


}

#ifndef DISABLE_JPEG_ENCODING
static int encodeData(mm_camera_super_buf_t* recvd_frame,
                      int frame_len,
                      int enqueued,
                      mm_camera_app_obj_t *pme)
{
    int ret = -1;
#if 0

    cam_ctrl_dimension_t dimension;
    struct msm_frame *postviewframe;
    struct msm_frame *mainframe;
    common_crop_t crop;
    cam_point_t main_crop_offset;
    cam_point_t thumb_crop_offset;
    int width, height;
    uint8_t *thumbnail_buf;
    uint32_t thumbnail_fd;

    omx_jpeg_encode_params encode_params;
    postviewframe = recvd_frame->snapshot.thumbnail.frame;
    mainframe = recvd_frame->snapshot.main.frame;
    dimension.orig_picture_dx = pme->dim.picture_width;
    dimension.orig_picture_dy = pme->dim.picture_height;
    dimension.thumbnail_width = pme->dim.ui_thumbnail_width;
    dimension.thumbnail_height = pme->dim.ui_thumbnail_height;
    dimension.main_img_format = pme->dim.main_img_format;
    dimension.thumb_format = pme->dim.thumb_format;

    CDBG("Setting callbacks, initializing encoder and start encoding.");
    my_cam_app.hal_lib.set_callbacks(snapshot_jpeg_fragment_cb, snapshot_jpeg_cb, pme,
                                     (void *)pme->jpeg_buf.frame[0].buffer, &JpegOffset);
    my_cam_app.hal_lib.omxJpegStart();
    my_cam_app.hal_lib.mm_jpeg_encoder_setMainImageQuality(85);

    /*TBD: Pass 0 as cropinfo for now as v4l2 doesn't provide
      cropinfo. It'll be changed later.*/
    memset(&crop,0,sizeof(common_crop_t));
    memset(&main_crop_offset,0,sizeof(cam_point_t));
    memset(&thumb_crop_offset,0,sizeof(cam_point_t));

    /*Fill in the encode parameters*/
    encode_params.dimension = (const cam_ctrl_dimension_t *)&dimension;
    encode_params.thumbnail_buf = (uint8_t *)postviewframe->buffer;
    encode_params.thumbnail_fd = postviewframe->fd;
    encode_params.thumbnail_offset = postviewframe->phy_offset;
    encode_params.snapshot_buf = (uint8_t *)mainframe->buffer;
    encode_params.snapshot_fd = mainframe->fd;
    encode_params.snapshot_offset = mainframe->phy_offset;
    encode_params.scaling_params = &crop;
    encode_params.exif_data = NULL;
    encode_params.exif_numEntries = 0;
    encode_params.a_cbcroffset = -1;
    encode_params.main_crop_offset = &main_crop_offset;
    encode_params.thumb_crop_offset = &thumb_crop_offset;

    if (!my_cam_app.hal_lib.omxJpegEncode(&encode_params)) {
        CDBG_ERROR("%s: Failure! JPEG encoder returned error.", __func__);
        ret = -1;
        goto end;
    }

    /* Save the pointer to the frame sent for encoding. we'll need it to
       tell kernel that we are done with the frame.*/
    mCurrentFrameEncoded = recvd_frame;

    end:
    CDBG("%s: X", __func__); 
#endif 
    return ret;
}

static int encodeDisplayAndSave(mm_camera_super_buf_t* recvd_frame,
                                int enqueued, mm_camera_app_obj_t *pme)
{
    int ret = -1;
#if 0


    CDBG("%s: Send frame for encoding", __func__);
    ret = encodeData(recvd_frame, pme->snapshot_buf.frame_len,
                     enqueued, pme);
    if (!ret) {
        CDBG_ERROR("%s: Failure configuring JPEG encoder", __func__);
    }

    LOGD("%s: X", __func__); 
#endif 
    return ret;
}
#endif //DISABLE_JPEG_ENCODING
static void mm_app_snapshot_notify_cb(mm_camera_super_buf_t *bufs,
                                      void *user_data)
{
#if 0
    mm_camera_app_obj_t *pme = user_data;
    int rc;

    CDBG("%s: BEGIN\n", __func__);
    snapshot_cnt++;
    mm_app_dump_snapshot_frame(bufs->snapshot.main.frame, pme->snapshot_buf.frame_len, TRUE, 0);
    mm_app_dump_snapshot_frame(bufs->snapshot.thumbnail.frame, pme->thumbnail_buf.frame_len, FALSE, 0);
#ifndef DISABLE_JPEG_ENCODING
    /* The recvd_frame structre we receive from lower library is a local
variable. So we'll need to save this structure so that we won't
be later pointing to garbage data when that variable goes out of
scope */
    mm_camera_ch_data_buf_t* frame =
    (mm_camera_ch_data_buf_t *)malloc(sizeof(mm_camera_ch_data_buf_t));
    if (frame == NULL) {
        CDBG_ERROR("%s: Error allocating memory to save received_frame structure.", __func__);
        goto error1;
    }
    memcpy(frame, bufs, sizeof(mm_camera_ch_data_buf_t));
    rc = encodeDisplayAndSave(frame, 0, pme);
    if (!rc) {
        CDBG_ERROR("%s: Error encoding buffer.", __func__);
        goto error;
    }
#endif //DISABLE_JPEG_ENCODING
    /* return buffer back for taking next snapshot */
    pme->cam->evt->buf_done(pme->cam, bufs);
    mm_app_snapshot_done();
/*
        CDBG("%s: calling mm_app_snapshot_done()\n", __func__);
        mm_app_snapshot_done();
*/
    CDBG("%s: END\n", __func__);
    return;
    error:
    /*if (frame != NULL)
      free(frame);*/
    error1:
    pme->cam->evt->buf_done(pme->cam, bufs);
    mm_app_snapshot_done();   
#endif 
    return;
}

static void mm_app_raw_snapshot_notify_cb(mm_camera_super_buf_t *bufs,
                                          void *user_data)
{
#if 0
    mm_camera_app_obj_t *pme = user_data;
    static int loop = 0;

    CDBG("%s: BEGIN\n", __func__);
    raw_snapshot_cnt++;
    mm_app_dump_snapshot_frame(bufs->def.frame, pme->raw_snapshot_buf.frame_len, TRUE, 1);
    /* return buffer back for taking next snapshot */
    pme->cam->evt->buf_done(pme->cam, bufs);
    CDBG("%s: calling mm_app_snapshot_done()\n", __func__);
    mm_app_snapshot_done();
    CDBG("%s: END\n", __func__); 
#endif 
}
static int mm_app_reg_snapshot_data_cb(int cam_id, int is_reg)
{
    int rc = MM_CAMERA_OK;
#if 0
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);


    CDBG("%s: BEGIN\n", __func__);
    if (is_reg) {
        rc = pme->cam->evt->register_buf_notify(pme->cam,
                                                MM_CAMERA_CH_SNAPSHOT,
                                                mm_app_snapshot_notify_cb,
                                                MM_CAMERA_REG_BUF_CB_INFINITE, 0,
                                                pme);
        if (rc != MM_CAMERA_OK) {
            CDBG("%s:register snapshot data notify cb err=%d\n",
                 __func__, rc);
            goto end;
        }
    } else {
        rc = pme->cam->evt->register_buf_notify(pme->cam,
                                                MM_CAMERA_CH_SNAPSHOT,
                                                NULL,
                                                (mm_camera_register_buf_cb_type_t)NULL,
                                                0, pme);
        if (rc != MM_CAMERA_OK) {
            CDBG("%s:unregister snapshot data notify cb err=%d\n",
                 __func__, rc);
            goto end;
        }
    }
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc); 
#endif 
    return rc;
}
static int mm_app_reg_raw_snapshot_data_cb(int cam_id, int is_reg)
{
    int rc = MM_CAMERA_OK;
#if 0
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);


    CDBG("%s: BEGIN\n", __func__);
    if (is_reg) {
        rc = pme->cam->evt->register_buf_notify(pme->cam,
                                                MM_CAMERA_CH_RAW,
                                                mm_app_raw_snapshot_notify_cb,
                                                MM_CAMERA_REG_BUF_CB_INFINITE, 0,
                                                pme);
        if (rc != MM_CAMERA_OK) {
            CDBG("%s:register raw snapshot data notify cb err=%d\n",
                 __func__, rc);
            goto end;
        }
    } else {
        rc = pme->cam->evt->register_buf_notify(pme->cam,
                                                MM_CAMERA_CH_RAW,
                                                NULL,
                                                (mm_camera_register_buf_cb_type_t)NULL, 0, pme);
        if (rc != MM_CAMERA_OK) {
            CDBG("%s:unregister raw snapshot data notify cb err=%d\n",
                 __func__, rc);
            goto end;
        }
    }
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc); 
#endif 
    return rc;
}

int mm_app_add_snapshot_stream(int cam_id)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id = pme->cam->ops->add_stream(pme->cam->camera_handle,pme->ch_id,
                                                                        NULL,pme,
                                                                        MM_CAMERA_SNAPSHOT_MAIN, 0);

    CDBG("Add Snapshot main is successfull stream ID = %d",pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id);
    if (!pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id) {
        CDBG_ERROR("%s:preview streaming err=%d\n", __func__, rc);
        rc = -1;
        goto end;
    }

    pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].id = pme->cam->ops->add_stream(pme->cam->camera_handle,pme->ch_id,
                                                                             NULL,pme,
                                                                             MM_CAMERA_SNAPSHOT_THUMBNAIL, 0);
    if (!pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].id) {
        CDBG_ERROR("%s:preview streaming err=%d\n", __func__, rc);
        rc = -1;
        goto end;
    }
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
    return rc;
}

void mm_app_set_snapshot_mode(int cam_id,int op_mode)
{
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    pme->cam->ops->set_parm(pme->cam->camera_handle,MM_CAMERA_PARM_OP_MODE, &op_mode);

}

int mm_app_config_snapshot_format(int cam_id)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    mm_app_set_snapshot_fmt(cam_id,&pme->stream[MM_CAMERA_SNAPSHOT_MAIN].str_config.fmt);

    mm_app_set_thumbnail_fmt(cam_id,&pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].str_config.fmt);

    pme->stream[MM_CAMERA_SNAPSHOT_MAIN].str_config.need_stream_on = 1;
    pme->stream[MM_CAMERA_SNAPSHOT_MAIN].str_config.num_of_bufs = 1;

    if (MM_CAMERA_OK != (rc = pme->cam->ops->config_stream(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id,
                                                           &pme->stream[MM_CAMERA_SNAPSHOT_MAIN].str_config))) {
        CDBG_ERROR("%s:preview streaming err=%d\n", __func__, rc);
        goto end;
    }

    pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].str_config.need_stream_on = 1;
    pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].str_config.num_of_bufs = 1;

    if (MM_CAMERA_OK != (rc = pme->cam->ops->config_stream(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].id,
                                                           &pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].str_config))) {
        CDBG_ERROR("%s:preview streaming err=%d\n", __func__, rc);
        goto end;
    }
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
    return rc;

}

int mm_app_streamon_snapshot(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int stream[2];
    mm_camera_bundle_attr_t attr;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    stream[0] = pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id;
    stream[1] = pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].id;

    attr.notify_mode = MM_CAMERA_SUPER_BUF_NOTIFY_BURST;
    attr.burst_num = 1;
    attr.look_back = 2;
    attr.post_frame_skip = 0;
    attr.water_mark = 2;

    if (MM_CAMERA_OK != (rc = pme->cam->ops->init_stream_bundle(
                                                               pme->cam->camera_handle,pme->ch_id,snapshot_raw_cb,pme,&attr,2,stream))) {
        CDBG_ERROR("%s:init_stream_bundle err=%d\n", __func__, rc);
        goto end;
    }

    if (MM_CAMERA_OK != (rc = pme->cam->ops->start_streams(pme->cam->camera_handle,pme->ch_id,
                                                           2, stream))) {
        CDBG_ERROR("%s:start_streams err=%d\n", __func__, rc);
        goto end;
    }

    if (MM_CAMERA_OK != (rc =pme->cam->ops->request_super_buf(pme->cam->camera_handle,pme->ch_id))) {
        CDBG_ERROR("%s:request_super_buf err=%d\n", __func__, rc);
        goto end;
    }
    pme->cam_state = CAMERA_STATE_SNAPSHOT;
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
    return rc;
}

int mm_app_streamoff_snapshot(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int stream[2];

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    stream[0] = pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id;
    stream[1] = pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].id;

    if (MM_CAMERA_OK != (rc = pme->cam->ops->stop_streams(pme->cam->camera_handle,pme->ch_id,2,&stream))) {
        CDBG_ERROR("%s : Snapshot Stream off Error",__func__);
        goto end;
    }
    CDBG("Stop snapshot main successfull");

    if (MM_CAMERA_OK != (rc = pme->cam->ops->destroy_stream_bundle(pme->cam->camera_handle,pme->ch_id))) {
        CDBG_ERROR("%s : Snapshot destroy_stream_bundle Error",__func__);
        goto end;
    }

    if (MM_CAMERA_OK != (rc = pme->cam->ops->del_stream(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id))) {
        CDBG_ERROR("%s : Snapshot main image del_stream Error",__func__);
        goto end;
    }
    CDBG("del_stream successfull");

    if (MM_CAMERA_OK != (rc = pme->cam->ops->del_stream(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].id))) {
        CDBG_ERROR("%s : Snapshot thumnail image del_stream Error",__func__);
        goto end;
    }
    CDBG("del_stream successfull");

    end:
    return rc;
}

int mm_app_start_snapshot(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int stream[2];
    int op_mode = 0; 

    mm_camera_bundle_attr_t attr;


    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    if (MM_CAMERA_OK != mm_app_stop_preview(cam_id)) {
        CDBG_ERROR("%s: Stop preview Failed cam_id=%d\n",__func__,cam_id);
        return -1;
    }

    op_mode = MM_CAMERA_OP_MODE_CAPTURE;
    mm_app_set_snapshot_mode(cam_id,op_mode);

    pme->cam->ops->prepare_snapshot(pme->cam->camera_handle,pme->ch_id,0);

    if (MM_CAMERA_OK != (rc = mm_app_add_snapshot_stream(cam_id))) {
        CDBG_ERROR("%s : Add Snapshot stream err",__func__);
    }

    if (MM_CAMERA_OK != (rc = mm_app_config_snapshot_format(cam_id))) {
        CDBG_ERROR("%s : Config Snapshot stream err",__func__);
    }

    if (MM_CAMERA_OK != (rc = mm_app_streamon_snapshot(cam_id))) {
        CDBG_ERROR("%s : Stream on Snapshot stream err",__func__);
    }

#if 0
    /*start OMX Jpeg encoder*/
#ifndef DISABLE_JPEG_ENCODING
    my_cam_app.hal_lib.omxJpegOpen();
#endif

#endif 
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);

    return rc;
}


int mm_app_stop_snapshot(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int stream[2];

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    stream[0] = pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id;
    stream[1] = pme->stream[MM_CAMERA_SNAPSHOT_THUMBNAIL].id;

    if (MM_CAMERA_OK != (rc = mm_app_streamoff_snapshot(cam_id))) {
        CDBG_ERROR("%s : Stream off Snapshot stream err",__func__);
    }
    pme->cam_state = CAMERA_STATE_OPEN;
#if 0
#ifndef DISABLE_JPEG_ENCODING
    my_cam_app.hal_lib.omxJpegClose();
#endif
#endif 
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);

    return rc;
}

int mm_app_start_raw_snapshot(int cam_id)
{
    int rc = MM_CAMERA_OK;
#if 0
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    mm_camera_channel_attr_t attr;


    attr.type = MM_CAMERA_CH_ATTR_RAW_STREAMING_TYPE;
    attr.raw_streaming_mode = MM_CAMERA_RAW_STREAMING_CAPTURE_SINGLE;

    if (MM_CAMERA_OK != (rc = mm_app_set_op_mode(cam_id, MM_CAMERA_OP_MODE_CAPTURE))) {
        CDBG("%s:mm_app_set_op_mode(op_mode=%d) err=%d\n", __func__,
             MM_CAMERA_OP_MODE_CAPTURE, rc);
        goto end;
    }
    if (MM_CAMERA_OK != (rc = mm_app_open_ch(cam_id, MM_CAMERA_CH_RAW))) {
        CDBG("%s:open raw snapshot channel err=%d\n", __func__, rc);
        goto end;
    }
    if (MM_CAMERA_OK != (rc = mm_app_set_raw_snapshot_fmt(cam_id))) {
        CDBG("%s:set raw snapshot format err=%d\n", __func__, rc);
        goto end;
    }
    mm_app_get_dim(cam_id, NULL);
    if (MM_CAMERA_OK != (rc = mm_app_prepare_raw_snapshot_buf(cam_id))) {
        CDBG("%s:reg raw snapshot buf err=%d\n", __func__, rc);
        goto end;
    }
    if (MM_CAMERA_OK != (rc = mm_app_reg_raw_snapshot_data_cb(cam_id, TRUE))) {
        CDBG("%s:reg raw snapshot data cb err=%d\n", __func__, rc);
    }
    if (MM_CAMERA_OK != (rc = pme->cam->ops->ch_set_attr(pme->cam, MM_CAMERA_CH_RAW, &attr))) {
        CDBG("%s:set raw capture attribute err=%d\n", __func__, rc);
        goto end;
    }
    if (MM_CAMERA_OK != (rc = pme->cam->ops->action(pme->cam, TRUE, MM_CAMERA_OPS_RAW, 0))) {
        CDBG("%s:snapshot streaming err=%d\n", __func__, rc);
        goto end;
    }
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
#endif
    return rc;
}

int mm_app_stop_raw_snapshot(int cam_id)
{
    int rc = MM_CAMERA_OK;
#if 0
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);


    CDBG("%s: BEGIN\n", __func__);
    if (MM_CAMERA_OK != (rc = pme->cam->ops->action(pme->cam, FALSE, MM_CAMERA_OPS_RAW, 0))) {
        CDBG("%s:stop raw snapshot streaming err=%d\n", __func__, rc);
        goto end;
    }
    if (MM_CAMERA_OK != (rc = mm_app_unprepare_raw_snapshot_buf(cam_id))) {
        CDBG("%s:mm_app_unprepare_raw_snapshot_buf err=%d\n", __func__, rc);
        return rc;
    }
    if (MM_CAMERA_OK != (rc = mm_app_reg_raw_snapshot_data_cb(cam_id, FALSE))) {
        CDBG("%s:mm_app_reg_raw_snapshot_data_cb err=%d\n", __func__, rc);
        return rc;
    }
    mm_app_close_ch(cam_id, MM_CAMERA_CH_RAW);
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
#endif
    return rc;
}

static void mm_app_snapshot_wait(int cam_id)
{
    pthread_mutex_lock(&g_s_mutex);
    if (false == g_status) {
        pthread_cond_wait(&g_s_cond_v, &g_s_mutex);
        g_status = false;
    }
    pthread_mutex_unlock(&g_s_mutex);
}

#if 0
int mm_stream_deinit_thumbnail_buf(uint32_t camera_handle,
                                   uint32_t ch_id, uint32_t stream_id,
                                   void *user_data, uint8_t num_bufs,
                                   mm_camera_buf_def_t *bufs)
{
    int i, rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = (mm_camera_app_obj_t *)user_data;

    for (i = 0; i < num_bufs; i++) {
        rc = my_cam_app.hal_lib.mm_camera_do_munmap_ion (pme->ionfd, &(pme->thumbnail_buf.frame[i].fd_data),
                                                         (void *)pme->thumbnail_buf.frame[i].buffer, pme->thumbnail_buf.frame_len);
        if (rc != MM_CAMERA_OK) {
            CDBG("%s: mm_camera_do_munmap err, pmem_fd = %d, rc = %d",
                 __func__, bufs[i].fd, rc);
        }
    }
    return rc;
}

int mm_stream_deinit_main_buf(uint32_t camera_handle,
                              uint32_t ch_id, uint32_t stream_id,
                              void *user_data, uint8_t num_bufs,
                              mm_camera_buf_def_t *bufs)
{
    int i, rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = (mm_camera_app_obj_t *)user_data;

    for (i = 0; i < num_bufs; i++) {
        rc = my_cam_app.hal_lib.mm_camera_do_munmap_ion (pme->ionfd, &(pme->snapshot_buf.frame[i].fd_data),
                                                         (void *)pme->snapshot_buf.frame[i].buffer, pme->snapshot_buf.frame_len);
        if (rc != MM_CAMERA_OK) {
            CDBG("%s: mm_camera_do_munmap err, pmem_fd = %d, rc = %d",
                 __func__, bufs[i].fd, rc);
        }
    }
    return rc;
}

int mm_stream_init_main_buf(uint32_t camera_handle,
                            uint32_t ch_id, uint32_t stream_id,
                            void *user_data,
                            mm_camera_frame_len_offset *frame_offset_info,
                            uint8_t num_bufs,
                            uint8_t *initial_reg_flag,
                            mm_camera_buf_def_t *bufs)
{
    int i,j,num_planes, frame_len, y_off, cbcr_off;
    uint32_t planes[VIDEO_MAX_PLANES];
    uint32_t pmem_addr = 0;

    mm_camera_app_obj_t *pme = (mm_camera_app_obj_t *)user_data;

    num_planes = frame_offset_info->num_planes;
    for ( i = 0; i < num_planes; i++) {
        planes[i] = frame_offset_info->mp[i].len;
    }

    frame_len = frame_offset_info->frame_len;
    y_off = frame_offset_info->mp[0].offset;
    cbcr_off = frame_offset_info->mp[1].offset;

    CDBG("Allocating main image Memory for %d buffers frame_len = %d",num_bufs,frame_offset_info->frame_len);

    for (i = 0; i < num_bufs ; i++) {
        int j;
        if (pme->cam_mode != RECORDER_MODE || pme->fullSizeSnapshot) {
            pme->snapshot_buf.reg[i] = 1;
            initial_reg_flag[i] = 1;
        } else {
            pme->snapshot_buf.reg[i] = 0;
            initial_reg_flag[i] = 0;
        }

        pme->snapshot_buf.frame_len = frame_len;

        pme->snapshot_buf.frame[i].ion_alloc.len = pme->snapshot_buf.frame_len;
        pme->snapshot_buf.frame[i].ion_alloc.flags =
        (0x1 << CAMERA_ION_HEAP_ID | 0x1 << ION_IOMMU_HEAP_ID);
        pme->snapshot_buf.frame[i].ion_alloc.align = 4096;

        pmem_addr = (unsigned long) my_cam_app.hal_lib.mm_camera_do_mmap_ion(pme->ionfd,
                                                                             &(pme->snapshot_buf.frame[i].ion_alloc), &(pme->snapshot_buf.frame[i].fd_data),
                                                                             &pme->snapshot_buf.frame[i].fd);

        pme->snapshot_buf.frame[i].buffer = pmem_addr;
        pme->snapshot_buf.frame[i].path = OUTPUT_TYPE_S;
        pme->snapshot_buf.frame[i].y_off = 0;
        pme->snapshot_buf.frame[i].cbcr_off = planes[0];
        pme->snapshot_buf.frame[i].phy_offset = 0;

        CDBG("Buffer allocated Successfully fd = %d",pme->snapshot_buf.frame[i].fd);

        bufs[i].fd = pme->snapshot_buf.frame[i].fd;
        //bufs[i].buffer = pmem_addr;
        bufs[i].frame_len = pme->snapshot_buf.frame[i].ion_alloc.len;
        bufs[i].num_planes = num_planes;

        bufs[i].frame = &pme->snapshot_buf.frame[i];

        /* Plane 0 needs to be set seperately. Set other planes
     * in a loop. */
        bufs[i].planes[0].length = planes[0];
        bufs[i].planes[0].m.userptr = bufs[i].fd;
        bufs[i].planes[0].data_offset = y_off;
        bufs[i].planes[0].reserved[0] = 0;
        //buf_def->buf.mp[i].frame_offset;
        for (j = 1; j < num_planes; j++) {
            bufs[i].planes[j].length = planes[j];
            bufs[i].planes[j].m.userptr = bufs[i].fd;
            bufs[i].planes[j].data_offset = cbcr_off;
            bufs[i].planes[j].reserved[0] =
            bufs[i].planes[j-1].reserved[0] +
            bufs[i].planes[j-1].length;
        }
    }
    return MM_CAMERA_OK;
}

int mm_stream_init_thumbnail_buf(uint32_t camera_handle,
                                 uint32_t ch_id, uint32_t stream_id,
                                 void *user_data,
                                 mm_camera_frame_len_offset *frame_offset_info,
                                 uint8_t num_bufs,
                                 uint8_t *initial_reg_flag,
                                 mm_camera_buf_def_t *bufs)
{
    int i,j,num_planes, frame_len, y_off, cbcr_off;
    uint32_t planes[VIDEO_MAX_PLANES];
    uint32_t pmem_addr = 0;

    mm_camera_app_obj_t *pme = (mm_camera_app_obj_t *)user_data;

    num_planes = frame_offset_info->num_planes;
    for ( i = 0; i < num_planes; i++) {
        planes[i] = frame_offset_info->mp[i].len;
    }

    frame_len = frame_offset_info->frame_len;
    y_off = frame_offset_info->mp[0].offset;
    cbcr_off = frame_offset_info->mp[1].offset;

    CDBG("Allocating thumanail image  Memory for %d buffers frame_len = %d",num_bufs,frame_offset_info->frame_len);

    for (i = 0; i < num_bufs ; i++) {
        int j;
        pme->thumbnail_buf.reg[i] = 1;
        initial_reg_flag[i] = 1;

        pme->thumbnail_buf.frame_len = frame_len;
        pme->thumbnail_buf.frame[i].ion_alloc.len = pme->thumbnail_buf.frame_len;
        pme->thumbnail_buf.frame[i].ion_alloc.flags =
        (0x1 << CAMERA_ION_HEAP_ID | 0x1 << ION_IOMMU_HEAP_ID);
        pme->thumbnail_buf.frame[i].ion_alloc.align = 4096;

        pmem_addr = (unsigned long) my_cam_app.hal_lib.mm_camera_do_mmap_ion(pme->ionfd,
                                                                             &(pme->thumbnail_buf.frame[i].ion_alloc), &(pme->thumbnail_buf.frame[i].fd_data),
                                                                             &pme->thumbnail_buf.frame[i].fd);

        pme->thumbnail_buf.frame[i].buffer = pmem_addr;
        pme->thumbnail_buf.frame[i].path = OUTPUT_TYPE_S;
        pme->thumbnail_buf.frame[i].y_off = 0;
        pme->thumbnail_buf.frame[i].cbcr_off = planes[0];
        pme->thumbnail_buf.frame[i].phy_offset = 0;

        CDBG("Buffer allocated Successfully fd = %d",pme->thumbnail_buf.frame[i].fd);

        bufs[i].fd = pme->thumbnail_buf.frame[i].fd;
        //bufs[i].buffer = pmem_addr;
        bufs[i].frame_len = pme->thumbnail_buf.frame[i].ion_alloc.len;
        bufs[i].num_planes = num_planes;

        bufs[i].frame = &pme->thumbnail_buf.frame[i];

        /* Plane 0 needs to be set seperately. Set other planes
     * in a loop. */
        bufs[i].planes[0].length = planes[0];
        bufs[i].planes[0].m.userptr = bufs[i].fd;
        bufs[i].planes[0].data_offset = y_off;
        bufs[i].planes[0].reserved[0] = 0;
        //buf_def->buf.mp[i].frame_offset;
        for (j = 1; j < num_planes; j++) {
            bufs[i].planes[j].length = planes[j];
            bufs[i].planes[j].m.userptr = bufs[i].fd;
            bufs[i].planes[j].data_offset = cbcr_off;
            bufs[i].planes[j].reserved[0] =
            bufs[i].planes[j-1].reserved[0] +
            bufs[i].planes[j-1].length;
        }
    }
    return MM_CAMERA_OK;
}
#endif
#if 0
int mm_app_bundle_zsl_stream(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int stream[3];
    mm_camera_bundle_attr_t attr;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    stream[0] = pme->stream[MM_CAMERA_PREVIEW].id;
    stream[1] = pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id;

    attr.notify_mode = MM_CAMERA_SUPER_BUF_NOTIFY_BURST;
    attr.burst_num = 1;
    attr.look_back = 2;
    attr.post_frame_skip = 0;
    attr.water_mark = 2;

    if (MM_CAMERA_OK != (rc = pme->cam->ops->init_stream_bundle(
                                                               pme->cam->camera_handle,pme->ch_id,mm_app_zsl_notify_cb,pme,&attr,2,stream))) {
        CDBG_ERROR("%s:init_stream_bundle err=%d\n", __func__, rc);
        rc = -1;
        goto end;
    }

    if (MM_CAMERA_OK != (rc = pme->cam->ops->start_streams(pme->cam->camera_handle,pme->ch_id,
                                                           2, stream))) {
        CDBG_ERROR("%s:start_streams err=%d\n", __func__, rc);
        rc = -1;
        goto end;
    }
    end:
    return rc;
}
#endif

static void mm_app_live_notify_cb(mm_camera_super_buf_t *bufs,
                                  void *user_data)
{

    int rc;
    int i = 0;
    mm_camera_buf_def_t *main_frame = NULL;
    mm_camera_app_obj_t *pme = NULL;
    CDBG("%s: BEGIN\n", __func__); 

    pme = (mm_camera_app_obj_t *)user_data;

    CDBG("%s : total streams = %d",__func__,bufs->num_bufs);
    main_frame = bufs->bufs[0];
    //thumb_frame = bufs->bufs[1];

    CDBG("mainframe frame_idx = %d fd = %d frame length = %d",main_frame->frame_idx,main_frame->fd,main_frame->frame_len);
    //CDBG("thumnail frame_idx = %d fd = %d frame length = %d",thumb_frame->frame_idx,thumb_frame->fd,thumb_frame->frame_len);

    //dumpFrameToFile(main_frame->frame,pme->dim.picture_width,pme->dim.picture_height,"main", 1);

    dumpFrameToFile(main_frame,pme->dim.picture_width,pme->dim.picture_height,"liveshot_main", 1);

    if (MM_CAMERA_OK != pme->cam->ops->qbuf(pme->cam->camera_handle,pme->ch_id,main_frame)) {
        CDBG_ERROR("%s: Failed in thumbnail Qbuf\n", __func__); 
    }
    /*if(MM_CAMERA_OK != pme->cam->ops->qbuf(pme->cam->camera_handle,pme->ch_id,thumb_frame))
    {
            CDBG_ERROR("%s: Failed in thumbnail Qbuf\n", __func__); 
    }*/

    mm_app_snapshot_done();
    CDBG("%s: END\n", __func__); 


}

int mm_app_prepare_live_snapshot(int cam_id)
{
    int rc = 0;
    int stream[1];
    mm_camera_bundle_attr_t attr;
    int value = 0;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    CDBG("%s:BEGIN, cam_id=%d\n",__func__,cam_id);

    stream[0] = pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id;
    //stream[1] = pme->stream[MM_CAMERA_PREVIEW].id;  //Need to clarify

    attr.notify_mode = MM_CAMERA_SUPER_BUF_NOTIFY_BURST;
    attr.burst_num = 1;
    attr.look_back = 2;
    attr.post_frame_skip = 0;
    attr.water_mark = 2;


    if (MM_CAMERA_OK != (rc = pme->cam->ops->set_parm(
                                                     pme->cam->camera_handle,MM_CAMERA_PARM_FULL_LIVESHOT, (void *)&value))) {
        CDBG_ERROR("%s: set dimension err=%d\n", __func__, rc);
    }

    if (MM_CAMERA_OK != (rc = pme->cam->ops->init_stream_bundle(
                                                               pme->cam->camera_handle,pme->ch_id,mm_app_live_notify_cb,pme,&attr,1,stream))) {
        CDBG_ERROR("%s:init_stream_bundle err=%d\n", __func__, rc);
        rc = -1;
        goto end;
    }

    if (MM_CAMERA_OK != (rc = pme->cam->ops->start_streams(pme->cam->camera_handle,pme->ch_id,
                                                           1, stream))) {
        CDBG_ERROR("%s:start_streams err=%d\n", __func__, rc);
        rc = -1;
        goto end;
    }


    end:
    CDBG("%s:END, cam_id=%d\n",__func__,cam_id);
    return rc;
}

int mm_app_unprepare_live_snapshot(int cam_id)
{
    int rc = 0;
    int stream[2];

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    CDBG("%s:BEGIN, cam_id=%d\n",__func__,cam_id);

    stream[0] = pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id;
    if (MM_CAMERA_OK != (rc = pme->cam->ops->stop_streams(pme->cam->camera_handle,pme->ch_id,1,&stream))) {
        CDBG_ERROR("%s : Snapshot Stream off Error",__func__);
        return -1;
    }

    if (MM_CAMERA_OK != (rc = pme->cam->ops->destroy_stream_bundle(pme->cam->camera_handle,pme->ch_id))) {
        CDBG_ERROR("%s : Snapshot destroy_stream_bundle Error",__func__);
        return -1;
    }
    CDBG("%s:END, cam_id=%d\n",__func__,cam_id);
    return rc;
}

int mm_app_take_live_snapshot(int cam_id)
{
    int rc = 0;
    int stream[3];
    mm_camera_bundle_attr_t attr;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s:BEGIN, cam_id=%d\n",__func__,cam_id);

    if (pme->cam_mode == RECORDER_MODE && 
        pme->cam_state == CAMERA_STATE_RECORD) {
        //Code to get live shot
        if (mm_app_prepare_live_snapshot(cam_id) != MM_CAMERA_OK) {
            CDBG_ERROR("%s: Failed prepare liveshot",__func__);
            return -1;
        }
        if (MM_CAMERA_OK != (rc =pme->cam->ops->request_super_buf(pme->cam->camera_handle,pme->ch_id))) {
            CDBG_ERROR("%s:request_super_buf err=%d\n", __func__, rc);
            return -1;
        }
        CDBG("%s:waiting images\n",__func__);
        mm_app_snapshot_wait(cam_id);

        if (MM_CAMERA_OK !=mm_app_unprepare_live_snapshot(cam_id)) {
            CDBG_ERROR("%s: Snapshot Stop error",__func__);
        }

    } else {
        CDBG_ERROR("%s: Should not come here for liveshot",__func__);
    }
    return rc;
}

int mm_app_take_picture_zsl(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int value = 1;
    int op_mode;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s: Take picture ZSL",__func__);

    if (MM_CAMERA_OK != (rc =pme->cam->ops->request_super_buf(pme->cam->camera_handle,pme->ch_id))) {
        CDBG_ERROR("%s:request_super_buf err=%d\n", __func__, rc);
        goto end;
    }

    CDBG("%s: Start ZSL Preview",__func__);

    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
    return rc;
}

int mm_app_take_picture_yuv(int cam_id)
{
    int rc;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s:BEGIN, cam_id=%d\n",__func__,cam_id);

    if (MM_CAMERA_OK != mm_app_start_snapshot(cam_id)) {
        CDBG_ERROR("%s: cam_id=%d\n",__func__,cam_id);      
        rc = -1;
        goto end;
    }

    CDBG("%s:waiting images\n",__func__);
    mm_app_snapshot_wait(cam_id);

    if (MM_CAMERA_OK !=mm_app_stop_snapshot(cam_id)) {
        CDBG_ERROR("%s: Snapshot Stop error",__func__);
    }

    preview:
    if (MM_CAMERA_OK != (rc = mm_app_start_preview(cam_id))) {
        CDBG("%s:preview start stream err=%d\n", __func__, rc);
    }
    end:
    CDBG("%s:END, cam_id=%d\n",__func__,cam_id);
    return rc;
}

int mm_app_take_zsl(int cam_id)
{
    int rc = MM_CAMERA_OK;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s:BEGIN, cam_id=%d\n",__func__,cam_id);

    if (pme->cam_mode == RECORDER_MODE) {
        switch (pme->cam_state) {
        case CAMERA_STATE_RECORD:
            if (MM_CAMERA_OK != mm_app_stop_video(cam_id)) {
                CDBG_ERROR("%s:Cannot stop video err=%d\n", __func__, rc);
                return -1;
            }
        case CAMERA_STATE_PREVIEW:
            if (MM_CAMERA_OK != mm_app_open_zsl(cam_id)) {
                CDBG_ERROR("%s: Cannot switch to camera mode=%d\n", __func__);
                return -1;
            }
            break;
        case CAMERA_STATE_SNAPSHOT:
        default:
            CDBG("%s: Cannot normal pciture in record mode\n", __func__);
            break; 
        }
    } else if (pme->cam_mode == CAMERA_MODE) {
        switch (pme->cam_state) {
        case CAMERA_STATE_PREVIEW:
            mm_app_open_zsl(cam_id);
            break;

        case CAMERA_STATE_SNAPSHOT:
        case CAMERA_STATE_RECORD:
        default:
            CDBG("%s: Cannot normal pciture in record mode\n", __func__);
            break; 
        }
    }

    if (pme->cam_mode == ZSL_MODE && pme->cam_state == CAMERA_STATE_PREVIEW) {
        mm_app_take_picture_zsl(cam_id);
    }
    return rc;
}

int mm_app_take_picture(int cam_id)
{
    int rc = 0;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s:BEGIN, cam_id=%d\n",__func__,cam_id);

    if (pme->cam_mode == RECORDER_MODE) {
        switch (pme->cam_state) {
        case CAMERA_STATE_RECORD:
            if (MM_CAMERA_OK != mm_app_stop_video(cam_id)) {
                CDBG_ERROR("%s:Cannot stop video err=%d\n", __func__, rc);
                return -1;
            }
        case CAMERA_STATE_PREVIEW:
            if (MM_CAMERA_OK != mm_app_open_camera(cam_id)) {
                CDBG_ERROR("%s: Cannot switch to camera mode=%d\n", __func__,rc);
                return -1;
            }
            break;
        case CAMERA_STATE_SNAPSHOT:
        default:
            CDBG("%s: Cannot normal pciture in record mode\n", __func__);
            break; 
        }
    } else if (pme->cam_mode == ZSL_MODE) {
        switch (pme->cam_state) {
        case CAMERA_STATE_PREVIEW:
            mm_app_open_camera(cam_id);
            break;

        case CAMERA_STATE_SNAPSHOT:
        case CAMERA_STATE_RECORD:
        default:
            CDBG("%s: Cannot normal pciture in record mode\n", __func__);
            break; 
        }
    }

    CDBG("%s : Takepicture : mode = %d state = %d, rc = %d",__func__,pme->cam_mode,pme->cam_state,rc);
    if (pme->cam_mode == CAMERA_MODE && pme->cam_state == CAMERA_STATE_PREVIEW) {
        mm_app_take_picture_yuv(cam_id);
    }
    return rc;
}

int mm_app_take_raw_picture(int cam_id)
{
    int rc;
#if 0
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s:BEGIN, cam_id=%d\n",__func__,cam_id);
    g_status = FALSE;
    if (MM_CAMERA_OK != (rc = pme->cam->ops->action(pme->cam, TRUE, MM_CAMERA_OPS_PREPARE_SNAPSHOT, 0))) {
        CDBG("%s:prepare snapshot err=%d\n", __func__, rc);
        goto end;
    }
    if (MM_CAMERA_OK != (rc = mm_app_stop_preview(cam_id))) {
        CDBG("%s:mm_app_stop_preview err=%d\n", __func__, rc);
        goto end;
    }
    if (MM_CAMERA_OK != mm_app_start_raw_snapshot(cam_id))
        goto preview;
    CDBG("%s:waiting images\n",__func__);
    mm_app_snapshot_wait(cam_id);
    CDBG("%s:calling mm_app_stop_snapshot() \n",__func__);
    mm_app_stop_raw_snapshot(cam_id);
    preview:
    mm_app_start_preview(cam_id);
    end:
    CDBG("%s:END, cam_id=%d\n",__func__,cam_id);
#endif
    return rc;
}

