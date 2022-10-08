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
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/msm_ion.h>
#include "mm_qcamera_app.h"


/*===========================================================================
 * FUNCTION    - mm_camera_do_mmap_ion -
 *
 * DESCRIPTION:
 *==========================================================================*/
uint8_t *mm_camera_do_mmap_ion(int ion_fd, struct ion_allocation_data *alloc,
                               struct ion_fd_data *ion_info_fd, int *mapFd)
{
    void *ret; /* returned virtual address */
    int rc = 0;
    struct ion_handle_data handle_data;

    /* to make it page size aligned */
    alloc->len = (alloc->len + 4095) & (~4095);

    rc = ioctl(ion_fd, ION_IOC_ALLOC, alloc);
    if (rc < 0) {
        CDBG_ERROR("ION allocation failed %s\n", strerror(errno));
        goto ION_ALLOC_FAILED;
    }

    ion_info_fd->handle = alloc->handle;
    rc = ioctl(ion_fd, ION_IOC_SHARE, ion_info_fd);
    if (rc < 0) {
        CDBG_ERROR("ION map failed %s\n", strerror(errno));
        goto ION_MAP_FAILED;
    }
    *mapFd = ion_info_fd->fd;
    ret = mmap(NULL,
               alloc->len,
               PROT_READ  | PROT_WRITE,
               MAP_SHARED,
               *mapFd,
               0);

    if (ret == MAP_FAILED) {
        CDBG_ERROR("ION_MMAP_FAILED: %s (%d)\n", strerror(errno), errno);
        goto ION_MAP_FAILED;
    }

    return ret;

    ION_MAP_FAILED:
    handle_data.handle = ion_info_fd->handle;
    ioctl(ion_fd, ION_IOC_FREE, &handle_data);
    ION_ALLOC_FAILED:
    return NULL;
}

/*===========================================================================
 * FUNCTION    - mm_camera_do_munmap_ion -
 *
 * DESCRIPTION:
 *==========================================================================*/
int mm_camera_do_munmap_ion (int ion_fd, struct ion_fd_data *ion_info_fd,
                             void *addr, size_t size)
{
    int rc = 0;
    rc = munmap(addr, size);
    close(ion_info_fd->fd);

    struct ion_handle_data handle_data;
    handle_data.handle = ion_info_fd->handle;
    ioctl(ion_fd, ION_IOC_FREE, &handle_data);
    return rc;
}


int mm_app_set_preview_fmt(int cam_id,mm_camera_image_fmt_t *fmt)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    fmt->meta_header = MM_CAMEAR_META_DATA_TYPE_DEF;
    fmt->fmt = pme->dim.prev_format;
    fmt->width = pme->dim.display_width;
    fmt->height = pme->dim.display_height;
    return rc;
}

//void dumpFrameToFile(struct msm_frame* newFrame, int w, int h, char* name, int main_422)
void dumpFrameToFile(mm_camera_buf_def_t* newFrame, int w, int h, char* name, int main_422)
{
    char buf[32];
    int file_fd;
    int i;
    char *ext = "yuv";
#if 0
    if ( newFrame != NULL) {
        char * str;
        snprintf(buf, sizeof(buf), "/data/%s.%s", name,ext);
        file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        if (file_fd < 0) {
            CDBG_ERROR("%s: cannot open file\n", __func__);
        } else {
            CDBG("%s: %d %d", __func__, newFrame->y_off, newFrame->cbcr_off);
            write(file_fd, (const void *)(newFrame->buffer+newFrame->y_off), w * h);
            write(file_fd, (const void *)
                  (newFrame->buffer + newFrame->cbcr_off), w * h / 2 * main_422);
            close(file_fd);
            CDBG("dump %s", buf);
        }
    }
#endif
    if ( newFrame != NULL) {
        char * str;
        snprintf(buf, sizeof(buf), "/data/%s.%s", name,ext);
        file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        if (file_fd < 0) {
            CDBG_ERROR("%s: cannot open file\n", __func__);
        } else {
            int y_off = newFrame->buffer + newFrame->planes[0].data_offset;
            //int cbcr_off = newFrame->buffer + newFrame->planes[1].data_offset;//newFrame->buffer + newFrame->planes[0].length;
            int cbcr_off = newFrame->buffer + newFrame->planes[0].length;
            CDBG("%s: Y_off = %p cbcr_off = %p", __func__, y_off,cbcr_off);
            CDBG("%s: Y_off length = %d cbcr_off length = %d", __func__, newFrame->planes[0].length,newFrame->planes[1].length);

            write(file_fd, (const void *)(y_off), newFrame->planes[0].length);
            write(file_fd, (const void *)(cbcr_off), 
                  (newFrame->planes[1].length * newFrame->num_planes));
            /*for(i = 1; i < newFrame->num_planes; i++) {
                CDBG("%s: CBCR = %d", __func__, newFrame->planes[j].data_offset);
                write(file_fd, (const void *)
                  (newFrame->planes[i].data_offset), w * h / 2 * main_422);
            }*/
            close(file_fd);
            CDBG("dump %s", buf);
        }
    }
}

int mm_app_open_camera(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int value = 0;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    if (pme->cam_mode == CAMERA_MODE) {
        return rc;
    }

    if (MM_CAMERA_OK != (rc = mm_app_stop_preview(cam_id))) {
        CDBG_ERROR("%s:Stop preview err=%d\n", __func__, rc);
        goto end;
    }

    pme->cam->ops->set_parm(pme->cam->camera_handle,MM_CAMERA_PARM_RECORDING_HINT, &value);

    if (MM_CAMERA_OK != (rc = mm_app_start_preview(cam_id))) {
        CDBG_ERROR("%s:Start preview err=%d\n", __func__, rc);
        goto end;
    }

    pme->cam_mode = CAMERA_MODE;
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
    return rc;
}


int mm_app_open_zsl(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int value = 0;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    if (pme->cam_mode == ZSL_MODE) {
        return rc;
    }

    if (MM_CAMERA_OK != (rc = mm_app_stop_preview(cam_id))) {
        CDBG_ERROR("%s:Stop preview err=%d\n", __func__, rc);
        goto end;
    }

    pme->cam->ops->set_parm(pme->cam->camera_handle,MM_CAMERA_PARM_RECORDING_HINT, &value);

    if (MM_CAMERA_OK != (rc = mm_app_start_preview_zsl(cam_id))) {
        CDBG_ERROR("%s:stream on preview err=%d\n", __func__, rc);
        goto end;
    }
    pme->cam_mode = ZSL_MODE;
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
    return rc;
}
#if 0
int mm_stream_deinit_preview_buf(uint32_t camera_handle,
                                 uint32_t ch_id, uint32_t stream_id,
                                 void *user_data, uint8_t num_bufs,
                                 mm_camera_buf_def_t *bufs)
{
    int i, rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = (mm_camera_app_obj_t *)user_data;
    for (i = 0; i < num_bufs; i++) {
        rc = my_cam_app.hal_lib.mm_camera_do_munmap_ion (pme->ionfd, &(pme->preview_buf.frame[i].fd_data),
                                                         (void *)pme->preview_buf.frame[i].buffer, bufs[i].frame_len);
        if (rc != MM_CAMERA_OK) {
            CDBG("%s: mm_camera_do_munmap err, pmem_fd = %d, rc = %d",
                 __func__, bufs[i].fd, rc);
        }
    }
    close(pme->ionfd);
    return rc;
}

int mm_stream_init_preview_buf(uint32_t camera_handle,
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

    CDBG("Allocating Preview Memory for %d buffers frame_len = %d",num_bufs,frame_offset_info->frame_len);

    for (i = 0; i < num_bufs ; i++) {
        int j;
        pme->preview_buf.reg[i] = 1;
        initial_reg_flag[i] = 1;

        pme->preview_buf.frame_len = frame_len;
        pme->preview_buf.frame[i].ion_alloc.len = pme->preview_buf.frame_len;
        pme->preview_buf.frame[i].ion_alloc.flags =
        (0x1 << CAMERA_ION_HEAP_ID | 0x1 << ION_IOMMU_HEAP_ID);
        pme->preview_buf.frame[i].ion_alloc.align = 4096;

        pmem_addr = (unsigned long) my_cam_app.hal_lib.mm_camera_do_mmap_ion(pme->ionfd,
                                                                             &(pme->preview_buf.frame[i].ion_alloc), &(pme->preview_buf.frame[i].fd_data),
                                                                             &pme->preview_buf.frame[i].fd);

        pme->preview_buf.frame[i].buffer = pmem_addr;
        pme->preview_buf.frame[i].path = OUTPUT_TYPE_P;
        pme->preview_buf.frame[i].y_off = 0;
        pme->preview_buf.frame[i].cbcr_off = planes[0];
        pme->preview_buf.frame[i].phy_offset = 0;

        CDBG("Buffer allocated Successfully fd = %d",pme->preview_buf.frame[i].fd);

        bufs[i].fd = pme->preview_buf.frame[i].fd;
        //bufs[i].buffer = pmem_addr;
        bufs[i].frame_len = pme->preview_buf.frame[i].ion_alloc.len;
        bufs[i].num_planes = num_planes;

        bufs[i].frame = &pme->preview_buf.frame[i];

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

int mm_stream_initbuf(uint32_t camera_handle,
                      uint32_t ch_id, uint32_t stream_id,
                      void *user_data,
                      mm_camera_frame_len_offset *frame_offset_info,
                      uint8_t num_bufs,
                      uint8_t *initial_reg_flag,
                      mm_camera_buf_def_t *bufs)
{
    int i,y_off, cbcr_off,num_planes;
    int plane_len;
    struct ion_allocation_data ion_alloc;
    struct ion_fd_data ion_info_fd;
    uint32_t pmem_addr = 0;
    uint32_t planes[VIDEO_MAX_PLANES];

    mm_camera_app_obj_t *pme = (mm_camera_app_obj_t *)user_data;

    num_planes = frame_offset_info->num_planes;
    if (num_planes == 1) {
        y_off = frame_offset_info->mp[0].offset;;
        cbcr_off = 0;
    } else {
        y_off = frame_offset_info->mp[0].offset;
        cbcr_off = frame_offset_info->mp[1].offset;
    }

    CDBG("%s: y_off = %d,cbcr_off = %d,num_planes = %d",__func__,y_off,
         cbcr_off,num_planes);

    pme->ionfd = open("/dev/ion", O_RDONLY);
    if (pme->ionfd < 0) {
        ALOGE("Ion dev open failed\n");
        ALOGE("Error is %s\n", strerror(errno));
    }

    for (i = 0; i < num_bufs ; i++) {
        int j;

        //if(pme->cam_mode != RECORDER_MODE || pme->fullSizeSnapshot) {
        initial_reg_flag[i] = 1;
        //}else{
        //initial_reg_flag[i] = 0;
        //}

        ion_alloc.len = frame_offset_info->frame_len;
        ion_alloc.flags =
        (0x1 << CAMERA_ION_HEAP_ID | 0x1 << ION_IOMMU_HEAP_ID);
        ion_alloc.align = 4096;

        /*bufs[i].buffer = my_cam_app.hal_lib.mm_camera_do_mmap_ion(pme->ionfd,
                       &ion_alloc, &ion_info_fd,
                       &bufs[i].fd);*/

        bufs[i].buffer = mm_camera_do_mmap_ion(pme->ionfd,
                                               &ion_alloc, &ion_info_fd,
                                               &bufs[i].fd);
        CDBG(" %s : Buffer allocated fd = %d, length = %d, y_off = %d cdcr_off = %d",
             __func__,bufs[i].fd,ion_alloc.len,y_off,cbcr_off);

        bufs[i].frame_len = ion_alloc.len;
        bufs[i].num_planes = num_planes;

        /* Plane 0 needs to be set seperately. Set other planes
             * in a loop. */
        bufs[i].planes[0].length = frame_offset_info->mp[0].len;
        bufs[i].planes[0].m.userptr = bufs[i].fd;
        bufs[i].planes[0].data_offset = y_off;
        bufs[i].planes[0].reserved[0] = 0;
        //buf_def->buf.mp[i].frame_offset;
        for (j = 1; j < num_planes; j++) {
            bufs[i].planes[j].length = frame_offset_info->mp[j].len;
            bufs[i].planes[j].m.userptr = bufs[i].fd;
            bufs[i].planes[j].data_offset = cbcr_off;
            bufs[i].planes[j].reserved[0] =
            bufs[i].planes[j-1].reserved[0] +
            bufs[i].planes[j-1].length;
        }
    }
    CDBG("%s: X",__func__);
    return MM_CAMERA_OK;
}

#if 0
int mm_stream_initbuf_1(uint32_t camera_handle,
                        uint32_t ch_id, uint32_t stream_id,
                        void *user_data,
                        mm_camera_frame_len_offset *frame_offset_info,
                        uint8_t num_bufs,
                        uint8_t *initial_reg_flag,
                        mm_camera_buf_def_t *bufs)
{
    int i;
    int streamType = 0;

    mm_camera_app_obj_t *pme = (mm_camera_app_obj_t *)user_data;

    CDBG("%s : E ", __FUNCTION__);

    for (i= 0; i < 5; i++) {
        if (pme->stream[i].id == stream_id) {
            CDBG("Allocate Memory for Stream %d",i);
            streamType = i;
            break;
        }
    }

    streamType = MM_CAMERA_PREVIEW;
    switch (streamType) {
    case MM_CAMERA_PREVIEW:
        mm_stream_init_preview_buf( camera_handle,
                                    ch_id, stream_id,
                                    user_data,
                                    frame_offset_info,
                                    num_bufs,
                                    initial_reg_flag,
                                    bufs);
        break;
    case MM_CAMERA_VIDEO:
        mm_stream_init_video_buf( camera_handle,
                                  ch_id, stream_id,
                                  user_data,
                                  frame_offset_info,
                                  num_bufs,
                                  initial_reg_flag,
                                  bufs);
        break;
    case MM_CAMERA_SNAPSHOT_MAIN:
        mm_stream_init_main_buf( camera_handle,
                                 ch_id, stream_id,
                                 user_data,
                                 frame_offset_info,
                                 num_bufs,
                                 initial_reg_flag,
                                 bufs);
        break;
    case MM_CAMERA_SNAPSHOT_THUMBNAIL:
        mm_stream_init_thumbnail_buf( camera_handle,
                                      ch_id, stream_id,
                                      user_data,
                                      frame_offset_info,
                                      num_bufs,
                                      initial_reg_flag,
                                      bufs);
        break;
    default:
        break;
    }

    CDBG(" %s : X ",__FUNCTION__);
    return MM_CAMERA_OK;
}
#endif
int mm_stream_deinitbuf(uint32_t camera_handle,
                        uint32_t ch_id, uint32_t stream_id,
                        void *user_data, uint8_t num_bufs,
                        mm_camera_buf_def_t *bufs)
{
    int i, rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = (mm_camera_app_obj_t *)user_data;

    CDBG("%s: E",__func__);

    for (i = 0; i < num_bufs; i++) {
        /*rc = my_cam_app.hal_lib.mm_camera_do_munmap_ion (pme->ionfd, &bufs[i].fd,
                   (void *)bufs[i].buffer, bufs[i].frame_len);*/
        rc = mm_camera_do_munmap_ion (pme->ionfd, &bufs[i].fd,
                                      (void *)bufs[i].buffer, bufs[i].frame_len);
    }
    close(pme->ionfd);
    CDBG("%s: X",__func__);
    return rc;
}

#if 0
static int mm_stream_deinitbuf_1(uint32_t camera_handle,
                                 uint32_t ch_id, uint32_t stream_id,
                                 void *user_data, uint8_t num_bufs,
                                 mm_camera_buf_def_t *bufs)
{
    int i, rc = MM_CAMERA_OK;
    int streamType = 0;
    mm_camera_app_obj_t *pme = (mm_camera_app_obj_t *)user_data;
    CDBG("%s: BEGIN",__func__);

    for (i= 0; i < 5; i++) {
        if (pme->stream[i].id == stream_id) {
            CDBG("Allocate Memory for Stream %d",i);
            streamType = i;
            break;
        }
    }
    streamType = MM_CAMERA_PREVIEW; 
    switch (streamType) {
    case MM_CAMERA_PREVIEW:
        mm_stream_deinit_preview_buf(camera_handle,
                                     ch_id, stream_id,
                                     user_data, num_bufs,
                                     bufs);
        break;
    case MM_CAMERA_VIDEO:
        mm_stream_deinit_video_buf(camera_handle,
                                   ch_id, stream_id,
                                   user_data, num_bufs,
                                   bufs);
        break;
    case MM_CAMERA_SNAPSHOT_MAIN:
        mm_stream_deinit_main_buf(camera_handle,
                                  ch_id, stream_id,
                                  user_data, num_bufs,
                                  bufs);
        break;
    case MM_CAMERA_SNAPSHOT_THUMBNAIL:
        mm_stream_deinit_thumbnail_buf(camera_handle,
                                       ch_id, stream_id,
                                       user_data, num_bufs,
                                       bufs);
        break;
    default:
        break;
    }

    /* zero out the buf stuct */
    CDBG("%s: END",__func__);
    return MM_CAMERA_OK;
}
#endif

void preview_cb_signal(mm_camera_app_obj_t *pme)
{
    if (pme->cam_state == CAMERA_STATE_PREVIEW) {
        mm_camera_app_done();
    }
}

void mm_app_preview_notify_cb(mm_camera_super_buf_t *bufs,
                              void *user_data)
{
    int rc;
    mm_camera_buf_def_t *frame = NULL;
    mm_camera_app_obj_t *pme = NULL;
    CDBG("%s: BEGIN\n", __func__); 
    frame = bufs->bufs[MM_CAMERA_PREVIEW] ;
    pme = (mm_camera_app_obj_t *)user_data;

    CDBG("%s: BEGIN - length=%d, frame idx = %d\n", __func__, frame->frame_len, frame->frame_idx);

    //dumpFrameToFile(frame->frame,pme->dim.display_width,pme->dim.display_height,"preview", 1);
    dumpFrameToFile(frame,pme->dim.display_width,pme->dim.display_height,"preview", 1);
    if (!my_cam_app.run_sanity) {
        if (0 != (rc = mm_app_dl_render(frame->fd, NULL))) {
            CDBG("%s:DL rendering err=%d, frame fd=%d,frame idx = %d\n",
                 __func__, rc, frame->fd, frame->frame_idx);
        }
    }

    CDBG("In CB function i/p = %p o/p = %p",bufs->bufs[MM_CAMERA_PREVIEW],frame);

    if (MM_CAMERA_OK != pme->cam->ops->qbuf(pme->cam->camera_handle,pme->ch_id,frame)) {
        CDBG_ERROR("%s: Failed in Preview Qbuf\n", __func__);
        return;
    }
    if (my_cam_app.run_sanity) {
        preview_cb_signal(pme);
    }
    CDBG("%s: END\n", __func__); 

}

static void mm_app_zsl_notify_cb(mm_camera_super_buf_t *bufs,
                                 void *user_data)
{
    int rc;
    int i = 0;
    mm_camera_buf_def_t *preview_frame = NULL;
    mm_camera_buf_def_t *main_frame = NULL;
    mm_camera_buf_def_t *thumb_frame = NULL;
    mm_camera_app_obj_t *pme = NULL;
    CDBG("%s: BEGIN\n", __func__); 

    pme = (mm_camera_app_obj_t *)user_data;

    CDBG("%s : total streams = %d",__func__,bufs->num_bufs);
    preview_frame = bufs->bufs[0] ;
    main_frame = bufs->bufs[1];
    thumb_frame = bufs->bufs[0];

    //dumpFrameToFile(preview_frame->frame,pme->dim.display_width,pme->dim.display_height,"preview", 1);
    dumpFrameToFile(preview_frame,pme->dim.display_width,pme->dim.display_height,"zsl_preview", 1);
    if (0 != (rc = mm_app_dl_render(preview_frame->fd, NULL))) {
        CDBG("%s:DL rendering err=%d, frame fd=%d,frame idx = %d\n",
             __func__, rc, preview_frame->fd, preview_frame->frame_idx);
    }

    if (bufs->num_bufs == 2 && main_frame != NULL) {
        CDBG("mainframe frame_idx = %d fd = %d frame length = %d",main_frame->frame_idx,main_frame->fd,main_frame->frame_len);
        CDBG("thumnail frame_idx = %d fd = %d frame length = %d",thumb_frame->frame_idx,thumb_frame->fd,thumb_frame->frame_len);

        //dumpFrameToFile(main_frame->frame,pme->dim.picture_width,pme->dim.picture_height,"main", 1);
        //dumpFrameToFile(thumb_frame->frame,pme->dim.thumbnail_width,pme->dim.thumbnail_height,"thumb", 1);

        dumpFrameToFile(main_frame,pme->dim.picture_width,pme->dim.picture_height,"zsl_main", 1);
        dumpFrameToFile(thumb_frame,pme->dim.thumbnail_width,pme->dim.thumbnail_height,"zsl_thumb", 1);

        if (MM_CAMERA_OK != pme->cam->ops->qbuf(pme->cam->camera_handle,pme->ch_id,main_frame)) {
            CDBG_ERROR("%s: Failed in thumbnail Qbuf\n", __func__); 
        }
    }

    if (MM_CAMERA_OK != pme->cam->ops->qbuf(pme->cam->camera_handle,pme->ch_id,preview_frame)) {
        CDBG_ERROR("%s: Failed in Preview Qbuf\n", __func__); 
    }
    CDBG("%s: END\n", __func__);
}

int mm_app_prepare_preview(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int op_mode;

    CDBG("%s: E",__func__);
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    pme->mem_cam->get_buf = mm_stream_initbuf;
    pme->mem_cam->put_buf = mm_stream_deinitbuf;
    pme->mem_cam->user_data = pme;

    op_mode = MM_CAMERA_OP_MODE_VIDEO;
    if (MM_CAMERA_OK != (rc = pme->cam->ops->set_parm(
                                                     pme->cam->camera_handle,MM_CAMERA_PARM_OP_MODE, &op_mode))) {
        CDBG_ERROR("%s: Set preview op mode error",__func__);
        goto end;
    }

    pme->stream[MM_CAMERA_PREVIEW].id = pme->cam->ops->add_stream(pme->cam->camera_handle,pme->ch_id,
                                                                  mm_app_preview_notify_cb,pme,
                                                                  MM_CAMERA_PREVIEW, 0);

    if (!pme->stream[MM_CAMERA_PREVIEW].id) {
        CDBG_ERROR("%s:Add stream preview error =%d\n", __func__, rc);
        rc = -1;
        goto end;
    }

    CDBG("%s :Add stream is successfull stream ID = %d",__func__,pme->stream[MM_CAMERA_PREVIEW].id);

    mm_app_set_preview_fmt(cam_id,&pme->stream[MM_CAMERA_PREVIEW].str_config.fmt);
    pme->stream[MM_CAMERA_PREVIEW].str_config.need_stream_on = 1;
    pme->stream[MM_CAMERA_PREVIEW].str_config.num_of_bufs = PREVIEW_BUF_NUM;

    if (MM_CAMERA_OK != (rc = pme->cam->ops->config_stream(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_PREVIEW].id,
                                                           &pme->stream[MM_CAMERA_PREVIEW].str_config))) {
        CDBG_ERROR("%s:preview streaming err=%d\n", __func__, rc);
        goto end;
    }
    end:
    return rc;
}

int mm_app_unprepare_preview(int cam_id)
{
    int rc = MM_CAMERA_OK;
    return rc;
}

int mm_app_streamon_preview(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int stream[2];
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    mm_camera_frame_len_offset frame_offset_info;

    stream[0] = pme->stream[MM_CAMERA_PREVIEW].id;
    stream[1] = 0;

    pme->cam->ops->get_stream_parm(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_PREVIEW].id,MM_CAMERA_STREAM_OFFSET,&frame_offset_info);
    ALOGE("DEBUG : length = %d",frame_offset_info.frame_len);
    if (MM_CAMERA_OK != (rc = pme->cam->ops->start_streams(pme->cam->camera_handle,pme->ch_id,1,&stream))) {
        CDBG_ERROR("%s : Start Stream preview Error",__func__);
        goto end;
    }
    pme->cam_state = CAMERA_STATE_PREVIEW;
    end:
    CDBG("%s: X rc = %d",__func__,rc);
    return rc;
}

int mm_app_prepare_preview_zsl(int cam_id)
{
    int rc = MM_CAMERA_OK;
    mm_camera_bundle_attr_t attr;
    int stream[3];

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    int op_mode = 0;

    op_mode = MM_CAMERA_OP_MODE_ZSL;
    if (MM_CAMERA_OK != (rc = pme->cam->ops->set_parm(
                                                     pme->cam->camera_handle,MM_CAMERA_PARM_OP_MODE, &op_mode))) {
        CDBG_ERROR("%s: Set preview op mode error",__func__);
        goto end;
    }

    pme->stream[MM_CAMERA_PREVIEW].id = pme->cam->ops->add_stream(pme->cam->camera_handle,pme->ch_id,
                                                                  mm_app_preview_notify_cb,pme,
                                                                  MM_CAMERA_PREVIEW, 0);

    if (!pme->stream[MM_CAMERA_PREVIEW].id) {
        CDBG_ERROR("%s:Add stream preview error =%d\n", __func__, rc);
        goto end;
    }

    CDBG("%s :Add stream is successfull stream ID = %d",__func__,pme->stream[MM_CAMERA_PREVIEW].id);

    mm_app_set_preview_fmt(cam_id,&pme->stream[MM_CAMERA_PREVIEW].str_config.fmt);
    pme->stream[MM_CAMERA_PREVIEW].str_config.need_stream_on = 1;
    pme->stream[MM_CAMERA_PREVIEW].str_config.num_of_bufs = PREVIEW_BUF_NUM;

    if (MM_CAMERA_OK != (rc = pme->cam->ops->config_stream(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_PREVIEW].id,
                                                           &pme->stream[MM_CAMERA_PREVIEW].str_config))) {
        CDBG_ERROR("%s:preview streaming err=%d\n", __func__, rc);
        goto end;
    }

    pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id = pme->cam->ops->add_stream(pme->cam->camera_handle,pme->ch_id,
                                                                        NULL,pme,
                                                                        MM_CAMERA_SNAPSHOT_MAIN, 0);

    CDBG("Add Snapshot main is successfull stream ID = %d",pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id);
    if (!pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id) {
        CDBG_ERROR("%s:preview streaming err=%d\n", __func__, rc);
        rc = -1;
        goto end;
    }

    pme->stream[MM_CAMERA_SNAPSHOT_MAIN].str_config.need_stream_on = 1;
    pme->stream[MM_CAMERA_SNAPSHOT_MAIN].str_config.num_of_bufs = 7;

    mm_app_set_snapshot_fmt(cam_id,&pme->stream[MM_CAMERA_SNAPSHOT_MAIN].str_config.fmt);


    if (MM_CAMERA_OK != (rc = pme->cam->ops->config_stream(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id,
                                                           &pme->stream[MM_CAMERA_SNAPSHOT_MAIN].str_config))) {
        CDBG_ERROR("%s:preview streaming err=%d\n", __func__, rc);
        goto end;
    }
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc); 
    return rc;
}

int mm_app_streamon_preview_zsl(int cam_id)
{
    int rc = MM_CAMERA_OK;
    mm_camera_bundle_attr_t attr;
    int stream[3];

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    int op_mode = 0;


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
    pme->cam_state = CAMERA_STATE_PREVIEW;
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc); 
    return rc;

}

int initDisplay()
{
    int rc = MM_CAMERA_OK;

    use_overlay_fb_display_driver();
    if (launch_camframe_fb_thread()) {
        CDBG_ERROR("%s:launch_camframe_fb_thread failed!\n", __func__);
        //rc = -MM_CAMERA_E_GENERAL;
    }
    CDBG("%s: launch_camframe_fb_thread done\n", __func__);
    return rc;
}

int deinitDisplay()
{
    /* stop the display thread */
    release_camframe_fb_thread();
    return MM_CAMERA_OK;
}

int mm_app_start_preview(int cam_id)
{
    int rc = MM_CAMERA_OK;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);
    int op_mode = 0;

    CDBG("pme = %p, pme->cam =%p, pme->cam->camera_handle = %d",
         pme,pme->cam,pme->cam->camera_handle);

    if (pme->cam_state == CAMERA_STATE_PREVIEW) {
        return rc;
    }

    if (!my_cam_app.run_sanity) {
        if (MM_CAMERA_OK != initDisplay()) {
            CDBG_ERROR("%s : Could not initalize display",__func__);
            goto end;
        }
    }

    if (MM_CAMERA_OK != (rc = mm_app_prepare_preview(cam_id))) {
        CDBG_ERROR("%s:Stream On Preview failed rc=%d\n", __func__, rc);
        goto end;
    }

    if (MM_CAMERA_OK != (rc = mm_app_streamon_preview(cam_id))) {
        CDBG_ERROR("%s:Stream On Preview failed rc=%d\n", __func__, rc);
        goto end;
    }

    end:
    CDBG("%s: END, rc=%d\n", __func__, rc); 
    return rc;
}

int mm_app_start_preview_zsl(int cam_id)
{
    int rc = MM_CAMERA_OK;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("pme = %p, pme->cam =%p, pme->cam->camera_handle = %d",
         pme,pme->cam,pme->cam->camera_handle);

    if (!my_cam_app.run_sanity) {
        if (MM_CAMERA_OK != initDisplay()) {
            CDBG_ERROR("%s : Could not initalize display",__func__);
            goto end;
        }
    }

    pme->mem_cam->get_buf = mm_stream_initbuf;
    pme->mem_cam->put_buf = mm_stream_deinitbuf;
    pme->mem_cam->user_data = pme;

    if (MM_CAMERA_OK != (rc = mm_app_prepare_preview_zsl(cam_id))) {
        CDBG_ERROR("%s:Prepare preview err=%d\n", __func__, rc);
        goto end;
    }

    if (MM_CAMERA_OK != (rc = mm_app_streamon_preview_zsl(cam_id))) {
        CDBG_ERROR("%s:stream on preview err=%d\n", __func__, rc);
        goto end;
    }

    /*if(MM_CAMERA_OK != (rc = mm_app_bundle_zsl_stream(cam_id))){
        CDBG_ERROR("%s: bundle and start of ZSl err=%d\n", __func__, rc);
        goto end;
    }*/

    end:
    CDBG("%s: END, rc=%d\n", __func__, rc); 
    return rc;
}

static int mm_app_streamoff_preview(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int stream[2];

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    stream[0] = pme->stream[MM_CAMERA_PREVIEW].id;
    stream[1] = 0;

    if (MM_CAMERA_OK != (rc = pme->cam->ops->stop_streams(pme->cam->camera_handle,pme->ch_id,1,&stream))) {
        CDBG_ERROR("%s : Preview Stream off Error",__func__);
        goto end;
    }

    if (MM_CAMERA_OK != (rc = pme->cam->ops->del_stream(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_PREVIEW].id))) {
        CDBG_ERROR("%s : Delete Stream Preview error",__func__);
        goto end;
    }
    CDBG("del_stream successfull");
    pme->cam_state = CAMERA_STATE_OPEN;
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);

    return rc;
}

static int mm_app_streamoff_preview_zsl(int cam_id)
{
    int rc = MM_CAMERA_OK;
    int stream[2];

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    stream[0] = pme->stream[MM_CAMERA_PREVIEW].id;
    stream[1] = pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id;

    if (MM_CAMERA_OK != (rc = pme->cam->ops->destroy_stream_bundle(pme->cam->camera_handle,pme->ch_id))) {
        CDBG_ERROR("%s : ZSL Snapshot destroy_stream_bundle Error",__func__);
        goto end;
    }

    if (MM_CAMERA_OK != (rc = pme->cam->ops->stop_streams(pme->cam->camera_handle,pme->ch_id,2,&stream))) {
        CDBG_ERROR("%s : Preview Stream off Error",__func__);
        goto end;
    }

    if (MM_CAMERA_OK != (rc = pme->cam->ops->del_stream(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_PREVIEW].id))) {
        CDBG_ERROR("%s : Delete Stream Preview error",__func__);
        goto end;
    }

    if (MM_CAMERA_OK != (rc = pme->cam->ops->del_stream(pme->cam->camera_handle,pme->ch_id,pme->stream[MM_CAMERA_SNAPSHOT_MAIN].id))) {
        CDBG_ERROR("%s : Delete Stream Preview error",__func__);
        goto end;
    }
    CDBG("del_stream successfull");
    pme->cam_state = CAMERA_STATE_OPEN;
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);

    return rc;
}

int startPreview(int cam_id)
{
    int rc = MM_CAMERA_OK;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s: Start Preview",__func__);

    if (pme->cam_mode == ZSL_MODE || pme->cam_mode == RECORDER_MODE) {
        switch (pme->cam_state) {
        case CAMERA_STATE_RECORD:
            if (MM_CAMERA_OK != mm_app_stop_video(cam_id)) {
                CDBG_ERROR("%s:Cannot stop video err=%d\n", __func__, rc);
                return -1;
            }
        case CAMERA_STATE_PREVIEW:
            if (MM_CAMERA_OK != mm_app_open_camera(cam_id)) {
                CDBG_ERROR("%s: Cannot switch to camera mode=%d\n", __func__);
                return -1;
            }
            break;
        case CAMERA_STATE_SNAPSHOT:
        default:
            break;
        }
    } else if (pme->cam_mode == CAMERA_MODE && pme->cam_state == CAMERA_STATE_OPEN) {

        if (MM_CAMERA_OK != (rc = mm_app_start_preview(cam_id))) {
            CDBG_ERROR("%s:preview streaming on err=%d\n", __func__, rc);
            return -1;
        }
    }
    CDBG("%s: END, rc=%d\n", __func__, rc);
    return rc;
}

int stopPreview(int cam_id)
{
    int rc = MM_CAMERA_OK;
    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    CDBG("%s : pme->cam_mode = %d, pme->cam_state = %d",__func__,pme->cam_mode,pme->cam_state);

    if (pme->cam_mode == CAMERA_MODE && pme->cam_state == CAMERA_STATE_PREVIEW) {
        if (MM_CAMERA_OK != (rc = mm_app_stop_preview(cam_id))) {
            CDBG("%s:streamoff preview err=%d\n", __func__, rc);
            goto end;
        }
    } else if (pme->cam_mode == ZSL_MODE && pme->cam_state == CAMERA_STATE_PREVIEW) {
        if (MM_CAMERA_OK != (rc = mm_app_stop_preview_zsl(cam_id))) {
            CDBG("%s:streamoff preview err=%d\n", __func__, rc);
            goto end;
        }
    } else if (pme->cam_mode == RECORDER_MODE && pme->cam_state == CAMERA_STATE_PREVIEW) {
        if (MM_CAMERA_OK != (rc = mm_app_stop_preview(cam_id))) {
            CDBG("%s:streamoff preview err=%d\n", __func__, rc);
            goto end;
        }
        mm_app_unprepare_video(cam_id);
    }
    end:
    return rc;
}

int mm_app_stop_preview(int cam_id)
{
    int rc = MM_CAMERA_OK;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    if (MM_CAMERA_OK != (rc = mm_app_streamoff_preview(cam_id))) {
        CDBG_ERROR("%s : Delete Stream Preview error",__func__);
        goto end;
    }

    CDBG("Stop Preview successfull");

    if (!my_cam_app.run_sanity) {
        deinitDisplay();
    }
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
    return rc;
}

int mm_app_stop_preview_zsl(int cam_id)
{
    int rc = MM_CAMERA_OK;

    mm_camera_app_obj_t *pme = mm_app_get_cam_obj(cam_id);

    if (MM_CAMERA_OK != (rc = mm_app_streamoff_preview_zsl(cam_id))) {
        CDBG_ERROR("%s : Delete Stream Preview error",__func__);
        goto end;
    }

    CDBG("Stop Preview successfull");
    if (!my_cam_app.run_sanity) {
        deinitDisplay();
    }
    end:
    CDBG("%s: END, rc=%d\n", __func__, rc);
    return rc;
}
