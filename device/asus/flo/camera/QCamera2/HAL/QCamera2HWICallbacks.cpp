/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#define LOG_TAG "QCamera2HWI"

#include <fcntl.h>
#include <utils/Errors.h>
#include <utils/Timers.h>
#include "QCamera2HWI.h"

namespace qcamera {

/*===========================================================================
 * FUNCTION   : zsl_channel_cb
 *
 * DESCRIPTION: helper function to handle ZSL superbuf callback directly from
 *              mm-camera-interface
 *
 * PARAMETERS :
 *   @recvd_frame : received super buffer
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : recvd_frame will be released after this call by caller, so if
 *             async operation needed for recvd_frame, it's our responsibility
 *             to save a copy for this variable to be used later.
 *==========================================================================*/
void QCamera2HardwareInterface::zsl_channel_cb(mm_camera_super_buf_t *recvd_frame,
                                               void *userdata)
{
    ALOGD("[KPI Perf] %s: E",__func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != recvd_frame->camera_handle){
       ALOGE("%s: camera obj not valid", __func__);
       return;
    }

    QCameraChannel *pChannel = pme->m_channels[QCAMERA_CH_TYPE_ZSL];
    if (pChannel == NULL ||
        pChannel->getMyHandle() != recvd_frame->ch_id) {
        ALOGE("%s: ZSL channel doesn't exist, return here", __func__);
        return;
    }

    // save a copy for the superbuf
    mm_camera_super_buf_t* frame =
               (mm_camera_super_buf_t *)malloc(sizeof(mm_camera_super_buf_t));
    if (frame == NULL) {
        ALOGE("%s: Error allocating memory to save received_frame structure.", __func__);
        pChannel->bufDone(recvd_frame);
        return;
    }
    *frame = *recvd_frame;

    // send to postprocessor
    pme->m_postprocessor.processData(frame);

    ALOGD("[KPI Perf] %s: X", __func__);
}

/*===========================================================================
 * FUNCTION   : capture_channel_cb_routine
 *
 * DESCRIPTION: helper function to handle snapshot superbuf callback directly from
 *              mm-camera-interface
 *
 * PARAMETERS :
 *   @recvd_frame : received super buffer
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : recvd_frame will be released after this call by caller, so if
 *             async operation needed for recvd_frame, it's our responsibility
 *             to save a copy for this variable to be used later.
*==========================================================================*/
void QCamera2HardwareInterface::capture_channel_cb_routine(mm_camera_super_buf_t *recvd_frame,
                                                           void *userdata)
{
    ALOGD("[KPI Perf] %s: E", __func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != recvd_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(recvd_frame);
        return;
    }

    QCameraChannel *pChannel = pme->m_channels[QCAMERA_CH_TYPE_CAPTURE];
    if (pChannel == NULL ||
        pChannel->getMyHandle() != recvd_frame->ch_id) {
        ALOGE("%s: Capture channel doesn't exist, return here", __func__);
        return;
    }

    // save a copy for the superbuf
    mm_camera_super_buf_t* frame =
               (mm_camera_super_buf_t *)malloc(sizeof(mm_camera_super_buf_t));
    if (frame == NULL) {
        ALOGE("%s: Error allocating memory to save received_frame structure.", __func__);
        pChannel->bufDone(recvd_frame);
        return;
    }
    *frame = *recvd_frame;

    // send to postprocessor
    pme->m_postprocessor.processData(frame);

/* START of test register face image for face authentication */
#ifdef QCOM_TEST_FACE_REGISTER_FACE
    static uint8_t bRunFaceReg = 1;

    if (bRunFaceReg > 0) {
        // find snapshot frame
        QCameraStream *main_stream = NULL;
        mm_camera_buf_def_t *main_frame = NULL;
        for (int i = 0; i < recvd_frame->num_bufs; i++) {
            QCameraStream *pStream =
                pChannel->getStreamByHandle(recvd_frame->bufs[i]->stream_id);
            if (pStream != NULL) {
                if (pStream->isTypeOf(CAM_STREAM_TYPE_SNAPSHOT) ||
                    pStream->isTypeOf(CAM_STREAM_TYPE_NON_ZSL_SNAPSHOT)) {
                    main_stream = pStream;
                    main_frame = recvd_frame->bufs[i];
                    break;
                }
            }
        }
        if (main_stream != NULL && main_frame != NULL) {
            int32_t faceId = -1;
            cam_pp_offline_src_config_t config;
            memset(&config, 0, sizeof(cam_pp_offline_src_config_t));
            config.num_of_bufs = 1;
            main_stream->getFormat(config.input_fmt);
            main_stream->getFrameDimension(config.input_dim);
            main_stream->getFrameOffset(config.input_buf_planes.plane_info);
            ALOGD("DEBUG: registerFaceImage E");
            int32_t rc = pme->registerFaceImage(main_frame->buffer, &config, faceId);
            ALOGD("DEBUG: registerFaceImage X, ret=%d, faceId=%d", rc, faceId);
            bRunFaceReg = 0;
        }
    }

#endif
/* END of test register face image for face authentication */

    ALOGD("[KPI Perf] %s: X", __func__);
}

/*===========================================================================
 * FUNCTION   : postproc_channel_cb_routine
 *
 * DESCRIPTION: helper function to handle postprocess superbuf callback directly from
 *              mm-camera-interface
 *
 * PARAMETERS :
 *   @recvd_frame : received super buffer
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : recvd_frame will be released after this call by caller, so if
 *             async operation needed for recvd_frame, it's our responsibility
 *             to save a copy for this variable to be used later.
*==========================================================================*/
void QCamera2HardwareInterface::postproc_channel_cb_routine(mm_camera_super_buf_t *recvd_frame,
                                                            void *userdata)
{
    ALOGD("[KPI Perf] %s: E", __func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != recvd_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(recvd_frame);
        return;
    }

    // save a copy for the superbuf
    mm_camera_super_buf_t* frame =
               (mm_camera_super_buf_t *)malloc(sizeof(mm_camera_super_buf_t));
    if (frame == NULL) {
        ALOGE("%s: Error allocating memory to save received_frame structure.", __func__);
        return;
    }
    *frame = *recvd_frame;

    // send to postprocessor
    pme->m_postprocessor.processPPData(frame);

    ALOGD("[KPI Perf] %s: X", __func__);
}

/*===========================================================================
 * FUNCTION   : preview_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle preview frame from preview stream in
 *              normal case with display.
 *
 * PARAMETERS :
 *   @super_frame : received super buffer
 *   @stream      : stream object
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : caller passes the ownership of super_frame, it's our
 *             responsibility to free super_frame once it's done. The new
 *             preview frame will be sent to display, and an older frame
 *             will be dequeued from display and needs to be returned back
 *             to kernel for future use.
 *==========================================================================*/
void QCamera2HardwareInterface::preview_stream_cb_routine(mm_camera_super_buf_t *super_frame,
                                                          QCameraStream * stream,
                                                          void *userdata)
{
    ALOGD("[KPI Perf] %s : BEGIN", __func__);
    int err = NO_ERROR;
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    QCameraGrallocMemory *memory = (QCameraGrallocMemory *)super_frame->bufs[0]->mem_info;

    if (pme == NULL) {
        ALOGE("%s: Invalid hardware object", __func__);
        free(super_frame);
        return;
    }
    if (memory == NULL) {
        ALOGE("%s: Invalid memory object", __func__);
        free(super_frame);
        return;
    }

    mm_camera_buf_def_t *frame = super_frame->bufs[0];
    if (NULL == frame) {
        ALOGE("%s: preview frame is NLUL", __func__);
        free(super_frame);
        return;
    }

    if (!pme->needProcessPreviewFrame()) {
        ALOGE("%s: preview is not running, no need to process", __func__);
        stream->bufDone(frame->buf_idx);
        free(super_frame);
        return;
    }

    if (pme->needDebugFps()) {
        pme->debugShowPreviewFPS();
    }

    int idx = frame->buf_idx;
    pme->dumpFrameToFile(frame->buffer, frame->frame_len,
                         frame->frame_idx, QCAMERA_DUMP_FRM_PREVIEW);

    // Display the buffer.
    int dequeuedIdx = memory->displayBuffer(idx);
    if (dequeuedIdx < 0 || dequeuedIdx >= memory->getCnt()) {
        ALOGD("%s: Invalid dequeued buffer index %d from display",
              __func__, dequeuedIdx);
    } else {
        // Return dequeued buffer back to driver
        err = stream->bufDone(dequeuedIdx);
        if ( err < 0) {
            ALOGE("stream bufDone failed %d", err);
        }
    }

    // Handle preview data callback
    if (pme->mDataCb != NULL && pme->msgTypeEnabledWithLock(CAMERA_MSG_PREVIEW_FRAME) > 0) {
        camera_memory_t *previewMem = NULL;
        camera_memory_t *data = NULL;
        int previewBufSize;
        cam_dimension_t preview_dim;
        cam_format_t previewFmt;
        stream->getFrameDimension(preview_dim);
        stream->getFormat(previewFmt);

        /* The preview buffer size in the callback should be (width*height*bytes_per_pixel)
         * As all preview formats we support, use 12 bits per pixel, buffer size = previewWidth * previewHeight * 3/2.
         * We need to put a check if some other formats are supported in future. */
        if ((previewFmt == CAM_FORMAT_YUV_420_NV21) ||
            (previewFmt == CAM_FORMAT_YUV_420_NV12) ||
            (previewFmt == CAM_FORMAT_YUV_420_YV12)) {
            if(previewFmt == CAM_FORMAT_YUV_420_YV12) {
                previewBufSize = ((preview_dim.width+15)/16) * 16 * preview_dim.height +
                                 ((preview_dim.width/2+15)/16) * 16* preview_dim.height;
                } else {
                    previewBufSize = preview_dim.width * preview_dim.height * 3/2;
                }
            if(previewBufSize != memory->getSize(idx)) {
                previewMem = pme->mGetMemory(memory->getFd(idx),
                           previewBufSize, 1, pme->mCallbackCookie);
                if (!previewMem || !previewMem->data) {
                    ALOGE("%s: mGetMemory failed.\n", __func__);
                } else {
                    data = previewMem;
                }
            } else
                data = memory->getMemory(idx, false);
        } else {
            data = memory->getMemory(idx, false);
            ALOGE("%s: Invalid preview format, buffer size in preview callback may be wrong.", __func__);
        }
        qcamera_callback_argm_t cbArg;
        memset(&cbArg, 0, sizeof(qcamera_callback_argm_t));
        cbArg.cb_type = QCAMERA_DATA_CALLBACK;
        cbArg.msg_type = CAMERA_MSG_PREVIEW_FRAME;
        cbArg.data = data;
        if ( previewMem ) {
            cbArg.user_data = previewMem;
            cbArg.release_cb = releaseCameraMemory;
        }
        cbArg.cookie = pme;
        pme->m_cbNotifier.notifyCallback(cbArg);
    }

    free(super_frame);
    ALOGD("[KPI Perf] %s : END", __func__);
    return;
}

/*===========================================================================
 * FUNCTION   : nodisplay_preview_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle preview frame from preview stream in
 *              no-display case
 *
 * PARAMETERS :
 *   @super_frame : received super buffer
 *   @stream      : stream object
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : caller passes the ownership of super_frame, it's our
 *             responsibility to free super_frame once it's done.
 *==========================================================================*/
void QCamera2HardwareInterface::nodisplay_preview_stream_cb_routine(
                                                          mm_camera_super_buf_t *super_frame,
                                                          QCameraStream *stream,
                                                          void * userdata)
{
    ALOGD("[KPI Perf] %s E",__func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != super_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(super_frame);
        return;
    }
    mm_camera_buf_def_t *frame = super_frame->bufs[0];
    if (NULL == frame) {
        ALOGE("%s: preview frame is NLUL", __func__);
        free(super_frame);
        return;
    }

    if (!pme->needProcessPreviewFrame()) {
        ALOGD("%s: preview is not running, no need to process", __func__);
        stream->bufDone(frame->buf_idx);
        free(super_frame);
        return;
    }

    if (pme->needDebugFps()) {
        pme->debugShowPreviewFPS();
    }

    QCameraMemory *previewMemObj = (QCameraMemory *)frame->mem_info;
    camera_memory_t *preview_mem = NULL;
    if (previewMemObj != NULL) {
        preview_mem = previewMemObj->getMemory(frame->buf_idx, false);
    }
    if (NULL != previewMemObj && NULL != preview_mem) {
        pme->dumpFrameToFile(frame->buffer, frame->frame_len,
                             frame->frame_idx, QCAMERA_DUMP_FRM_PREVIEW);

        if (pme->needProcessPreviewFrame() &&
            pme->mDataCb != NULL &&
            pme->msgTypeEnabledWithLock(CAMERA_MSG_PREVIEW_FRAME) > 0 ) {
            qcamera_callback_argm_t cbArg;
            memset(&cbArg, 0, sizeof(qcamera_callback_argm_t));
            cbArg.cb_type = QCAMERA_DATA_CALLBACK;
            cbArg.msg_type = CAMERA_MSG_PREVIEW_FRAME;
            cbArg.data = preview_mem;
            int user_data = frame->buf_idx;
            cbArg.user_data = ( void * ) user_data;
            cbArg.cookie = stream;
            cbArg.release_cb = returnStreamBuffer;
            pme->m_cbNotifier.notifyCallback(cbArg);
        } else {
            stream->bufDone(frame->buf_idx);
        }
    }
    free(super_frame);
    ALOGD("[KPI Perf] %s X",__func__);
}

/*===========================================================================
 * FUNCTION   : postview_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle post frame from postview stream
 *
 * PARAMETERS :
 *   @super_frame : received super buffer
 *   @stream      : stream object
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : caller passes the ownership of super_frame, it's our
 *             responsibility to free super_frame once it's done.
 *==========================================================================*/
void QCamera2HardwareInterface::postview_stream_cb_routine(mm_camera_super_buf_t *super_frame,
                                                           QCameraStream *stream,
                                                           void *userdata)
{
    int err = NO_ERROR;
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    QCameraGrallocMemory *memory = (QCameraGrallocMemory *)super_frame->bufs[0]->mem_info;

    if (pme == NULL) {
        ALOGE("%s: Invalid hardware object", __func__);
        free(super_frame);
        return;
    }
    if (memory == NULL) {
        ALOGE("%s: Invalid memory object", __func__);
        free(super_frame);
        return;
    }

    ALOGD("[KPI Perf] %s : BEGIN", __func__);

    mm_camera_buf_def_t *frame = super_frame->bufs[0];
    if (NULL == frame) {
        ALOGE("%s: preview frame is NLUL", __func__);
        free(super_frame);
        return;
    }

    QCameraMemory *memObj = (QCameraMemory *)frame->mem_info;
    if (NULL != memObj) {
        pme->dumpFrameToFile(frame->buffer, frame->frame_len,
                             frame->frame_idx, QCAMERA_DUMP_FRM_THUMBNAIL);
    }

    // Display the buffer.
    int dequeuedIdx = memory->displayBuffer(frame->buf_idx);
    if (dequeuedIdx < 0 || dequeuedIdx >= memory->getCnt()) {
        ALOGD("%s: Invalid dequeued buffer index %d",
              __func__, dequeuedIdx);
        free(super_frame);
        return;
    }

    // Return dequeued buffer back to driver
    err = stream->bufDone(dequeuedIdx);
    if ( err < 0) {
        ALOGE("stream bufDone failed %d", err);
    }

    free(super_frame);
    ALOGD("[KPI Perf] %s : END", __func__);
    return;
}

/*===========================================================================
 * FUNCTION   : video_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle video frame from video stream
 *
 * PARAMETERS :
 *   @super_frame : received super buffer
 *   @stream      : stream object
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : caller passes the ownership of super_frame, it's our
 *             responsibility to free super_frame once it's done. video
 *             frame will be sent to video encoder. Once video encoder is
 *             done with the video frame, it will call another API
 *             (release_recording_frame) to return the frame back
 *==========================================================================*/
void QCamera2HardwareInterface::video_stream_cb_routine(mm_camera_super_buf_t *super_frame,
                                                        QCameraStream */*stream*/,
                                                        void *userdata)
{
    ALOGD("[KPI Perf] %s : BEGIN", __func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != super_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(super_frame);
        return;
    }
    mm_camera_buf_def_t *frame = super_frame->bufs[0];

    if (pme->needDebugFps()) {
        pme->debugShowVideoFPS();
    }

    ALOGE("%s: Stream(%d), Timestamp: %ld %ld",
          __func__,
          frame->stream_id,
          frame->ts.tv_sec,
          frame->ts.tv_nsec);

    nsecs_t timeStamp = nsecs_t(frame->ts.tv_sec) * 1000000000LL + frame->ts.tv_nsec;
    ALOGE("Send Video frame to services/encoder TimeStamp : %lld", timeStamp);
    QCameraMemory *videoMemObj = (QCameraMemory *)frame->mem_info;
    camera_memory_t *video_mem = NULL;
    if (NULL != videoMemObj) {
        video_mem = videoMemObj->getMemory(frame->buf_idx, (pme->mStoreMetaDataInFrame > 0)? true : false);
    }
    if (NULL != videoMemObj && NULL != video_mem) {
        pme->dumpFrameToFile(frame->buffer, frame->frame_len,
                             frame->frame_idx, QCAMERA_DUMP_FRM_VIDEO);
        if ((pme->mDataCbTimestamp != NULL) &&
            pme->msgTypeEnabledWithLock(CAMERA_MSG_VIDEO_FRAME) > 0) {
            qcamera_callback_argm_t cbArg;
            memset(&cbArg, 0, sizeof(qcamera_callback_argm_t));
            cbArg.cb_type = QCAMERA_DATA_TIMESTAMP_CALLBACK;
            cbArg.msg_type = CAMERA_MSG_VIDEO_FRAME;
            cbArg.data = video_mem;
            cbArg.timestamp = timeStamp;
            pme->m_cbNotifier.notifyCallback(cbArg);
        }
    }
    free(super_frame);
    ALOGD("[KPI Perf] %s : END", __func__);
}

/*===========================================================================
 * FUNCTION   : snapshot_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle snapshot frame from snapshot stream
 *
 * PARAMETERS :
 *   @super_frame : received super buffer
 *   @stream      : stream object
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : caller passes the ownership of super_frame, it's our
 *             responsibility to free super_frame once it's done. For
 *             snapshot, it need to send to postprocessor for jpeg
 *             encoding, therefore the ownership of super_frame will be
 *             hand to postprocessor.
 *==========================================================================*/
void QCamera2HardwareInterface::snapshot_stream_cb_routine(mm_camera_super_buf_t *super_frame,
                                                           QCameraStream * /*stream*/,
                                                           void *userdata)
{
    ALOGD("[KPI Perf] %s: E", __func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != super_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(super_frame);
        return;
    }

    pme->m_postprocessor.processData(super_frame);

    ALOGD("[KPI Perf] %s: X", __func__);
}

/*===========================================================================
 * FUNCTION   : raw_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle raw dump frame from raw stream
 *
 * PARAMETERS :
 *   @super_frame : received super buffer
 *   @stream      : stream object
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : caller passes the ownership of super_frame, it's our
 *             responsibility to free super_frame once it's done. For raw
 *             frame, there is no need to send to postprocessor for jpeg
 *             encoding. this function will play shutter and send the data
 *             callback to upper layer. Raw frame buffer will be returned
 *             back to kernel, and frame will be free after use.
 *==========================================================================*/
void QCamera2HardwareInterface::raw_stream_cb_routine(mm_camera_super_buf_t * super_frame,
                                                      QCameraStream * /*stream*/,
                                                      void * userdata)
{
    ALOGD("[KPI Perf] %s : BEGIN", __func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != super_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(super_frame);
        return;
    }

    pme->m_postprocessor.processRawData(super_frame);
    ALOGD("[KPI Perf] %s : END", __func__);
}

/*===========================================================================
 * FUNCTION   : metadata_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle metadata frame from metadata stream
 *
 * PARAMETERS :
 *   @super_frame : received super buffer
 *   @stream      : stream object
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : caller passes the ownership of super_frame, it's our
 *             responsibility to free super_frame once it's done. Metadata
 *             could have valid entries for face detection result or
 *             histogram statistics information.
 *==========================================================================*/
void QCamera2HardwareInterface::metadata_stream_cb_routine(mm_camera_super_buf_t * super_frame,
                                                           QCameraStream * stream,
                                                           void * userdata)
{
    ALOGV("[KPI Perf] %s : BEGIN", __func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != super_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(super_frame);
        return;
    }

    mm_camera_buf_def_t *frame = super_frame->bufs[0];
    cam_metadata_info_t *pMetaData = (cam_metadata_info_t *)frame->buffer;

    if (pMetaData->is_faces_valid) {
        if (pMetaData->faces_data.num_faces_detected > MAX_ROI) {
            ALOGE("%s: Invalid number of faces %d",
                __func__, pMetaData->faces_data.num_faces_detected);
        } else {
            // process face detection result
            ALOGD("[KPI Perf] %s: Number of faces detected %d",__func__,pMetaData->faces_data.num_faces_detected);
            pme->processFaceDetectionResult(&pMetaData->faces_data);
        }
    }

    if (pMetaData->is_stats_valid) {
        // process histogram statistics info
        pme->processHistogramStats(pMetaData->stats_data);
    }

    if (pMetaData->is_focus_valid) {
        // process focus info
        qcamera_sm_internal_evt_payload_t *payload =
            (qcamera_sm_internal_evt_payload_t *)malloc(sizeof(qcamera_sm_internal_evt_payload_t));
        if (NULL != payload) {
            memset(payload, 0, sizeof(qcamera_sm_internal_evt_payload_t));
            payload->evt_type = QCAMERA_INTERNAL_EVT_FOCUS_UPDATE;
            payload->focus_data = pMetaData->focus_data;
            int32_t rc = pme->processEvt(QCAMERA_SM_EVT_EVT_INTERNAL, payload);
            if (rc != NO_ERROR) {
                ALOGE("%s: processEVt failed", __func__);
                free(payload);
                payload = NULL;

            }
        } else {
            ALOGE("%s: No memory for qcamera_sm_internal_evt_payload_t", __func__);
        }
    }

    if (pMetaData->is_crop_valid) {
        if (pMetaData->crop_data.num_of_streams > MAX_NUM_STREAMS) {
            ALOGE("%s: Invalid num_of_streams %d in crop_data", __func__,
                pMetaData->crop_data.num_of_streams);
        } else {
            pme->processZoomEvent(pMetaData->crop_data);
        }
    }

    if (pMetaData->is_prep_snapshot_done_valid) {
        qcamera_sm_internal_evt_payload_t *payload =
            (qcamera_sm_internal_evt_payload_t *)malloc(sizeof(qcamera_sm_internal_evt_payload_t));
        if (NULL != payload) {
            memset(payload, 0, sizeof(qcamera_sm_internal_evt_payload_t));
            payload->evt_type = QCAMERA_INTERNAL_EVT_PREP_SNAPSHOT_DONE;
            payload->prep_snapshot_state = pMetaData->prep_snapshot_done_state;
            int32_t rc = pme->processEvt(QCAMERA_SM_EVT_EVT_INTERNAL, payload);
            if (rc != NO_ERROR) {
                ALOGE("%s: processEVt failed", __func__);
                free(payload);
                payload = NULL;

            }
        } else {
            ALOGE("%s: No memory for qcamera_sm_internal_evt_payload_t", __func__);
        }
    }

    stream->bufDone(frame->buf_idx);
    free(super_frame);

    ALOGV("[KPI Perf] %s : END", __func__);
}

/*===========================================================================
 * FUNCTION   : reprocess_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle reprocess frame from reprocess stream
                (after reprocess, e.g., ZSL snapshot frame after WNR if
 *              WNR is enabled)
 *
 * PARAMETERS :
 *   @super_frame : received super buffer
 *   @stream      : stream object
 *   @userdata    : user data ptr
 *
 * RETURN    : None
 *
 * NOTE      : caller passes the ownership of super_frame, it's our
 *             responsibility to free super_frame once it's done. In this
 *             case, reprocessed frame need to be passed to postprocessor
 *             for jpeg encoding.
 *==========================================================================*/
void QCamera2HardwareInterface::reprocess_stream_cb_routine(mm_camera_super_buf_t * super_frame,
                                                            QCameraStream * /*stream*/,
                                                            void * userdata)
{
    ALOGD("[KPI Perf] %s: E", __func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != super_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(super_frame);
        return;
    }

    pme->m_postprocessor.processPPData(super_frame);

    ALOGD("[KPI Perf] %s: X", __func__);
}

/*===========================================================================
 * FUNCTION   : dumpFrameToFile
 *
 * DESCRIPTION: helper function to dump frame into file for debug purpose.
 *
 * PARAMETERS :
 *    @data : data ptr
 *    @size : length of data buffer
 *    @index : identifier for data
 *    @dump_type : type of the frame to be dumped. Only such
 *                 dump type is enabled, the frame will be
 *                 dumped into a file.
 *
 * RETURN     : None
 *==========================================================================*/
void QCamera2HardwareInterface::dumpFrameToFile(const void *data,
                                                uint32_t size,
                                                int index,
                                                int dump_type)
{
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.camera.dumpimg", value, "0");
    int32_t enabled = atoi(value);
    int frm_num = 0;
    uint32_t skip_mode = 0;

    char buf[32];
    cam_dimension_t dim;
    memset(buf, 0, sizeof(buf));
    memset(&dim, 0, sizeof(dim));

    if(enabled & QCAMERA_DUMP_FRM_MASK_ALL) {
        if((enabled & dump_type) && data) {
            frm_num = ((enabled & 0xffff0000) >> 16);
            if(frm_num == 0) {
                frm_num = 10; //default 10 frames
            }
            if(frm_num > 256) {
                frm_num = 256; //256 buffers cycle around
            }
            skip_mode = ((enabled & 0x0000ff00) >> 8);
            if(skip_mode == 0) {
                skip_mode = 1; //no-skip
            }

            if( mDumpSkipCnt % skip_mode == 0) {
                if((frm_num == 256) && (mDumpFrmCnt >= frm_num)) {
                    // reset frame count if cycling
                    mDumpFrmCnt = 0;
                }
                if (mDumpFrmCnt >= 0 && mDumpFrmCnt <= frm_num) {
                    switch (dump_type) {
                    case QCAMERA_DUMP_FRM_PREVIEW:
                        {
                            mParameters.getStreamDimension(CAM_STREAM_TYPE_PREVIEW, dim);
                            snprintf(buf, sizeof(buf), "/data/%dp_%dx%d_%d.yuv",
                                     mDumpFrmCnt, dim.width, dim.height, index);
                        }
                        break;
                    case QCAMERA_DUMP_FRM_THUMBNAIL:
                        {
                        mParameters.getStreamDimension(CAM_STREAM_TYPE_POSTVIEW, dim);
                        snprintf(buf, sizeof(buf), "/data/%dt_%dx%d_%d.yuv",
                                 mDumpFrmCnt, dim.width, dim.height, index);
                        }
                        break;
                    case QCAMERA_DUMP_FRM_SNAPSHOT:
                        {
                            if (mParameters.isZSLMode())
                                mParameters.getStreamDimension(CAM_STREAM_TYPE_SNAPSHOT, dim);
                            else
                                mParameters.getStreamDimension(CAM_STREAM_TYPE_NON_ZSL_SNAPSHOT, dim);
                            snprintf(buf, sizeof(buf), "/data/%ds_%dx%d_%d.yuv",
                                     mDumpFrmCnt, dim.width, dim.height, index);
                        }
                    break;
                    case QCAMERA_DUMP_FRM_VIDEO:
                        {
                            mParameters.getStreamDimension(CAM_STREAM_TYPE_VIDEO, dim);
                            snprintf(buf, sizeof(buf), "/data/%dv_%dx%d_%d.yuv",
                                     mDumpFrmCnt, dim.width, dim.height, index);
                        }
                        break;
                    case QCAMERA_DUMP_FRM_RAW:
                        {
                            mParameters.getStreamDimension(CAM_STREAM_TYPE_RAW, dim);
                            snprintf(buf, sizeof(buf), "/data/%dr_%dx%d_%d.yuv",
                                     mDumpFrmCnt, dim.width, dim.height, index);
                        }
                        break;
                    case QCAMERA_DUMP_FRM_JPEG:
                        {
                            if (mParameters.isZSLMode())
                                mParameters.getStreamDimension(CAM_STREAM_TYPE_SNAPSHOT, dim);
                            else
                                mParameters.getStreamDimension(CAM_STREAM_TYPE_NON_ZSL_SNAPSHOT, dim);
                            snprintf(buf, sizeof(buf), "/data/%dj_%dx%d_%d.yuv",
                                     mDumpFrmCnt, dim.width, dim.height, index);
                        }
                        break;
                    default:
                        ALOGE("%s: Not supported for dumping stream type %d",
                              __func__, dump_type);
                        return;
                    }

                    ALOGD("dump %s size =%d, data = %p", buf, size, data);
                    int file_fd = open(buf, O_RDWR | O_CREAT, 0777);
                    if (file_fd > 0) {
                        int written_len = write(file_fd, data, size);
                        ALOGD("%s: written number of bytes %d\n", __func__, written_len);
                        close(file_fd);
                    } else {
                        ALOGE("%s: fail t open file for image dumping", __func__);
                    }
                    mDumpFrmCnt++;
                }
            }
            mDumpSkipCnt++;
        }
    } else {
        mDumpFrmCnt = 0;
    }
}

/*===========================================================================
 * FUNCTION   : debugShowVideoFPS
 *
 * DESCRIPTION: helper function to log video frame FPS for debug purpose.
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *==========================================================================*/
void QCamera2HardwareInterface::debugShowVideoFPS()
{
    static int n_vFrameCount = 0;
    static int n_vLastFrameCount = 0;
    static nsecs_t n_vLastFpsTime = 0;
    static float n_vFps = 0;
    n_vFrameCount++;
    nsecs_t now = systemTime();
    nsecs_t diff = now - n_vLastFpsTime;
    if (diff > ms2ns(250)) {
        n_vFps =  ((n_vFrameCount - n_vLastFrameCount) * float(s2ns(1))) / diff;
        ALOGE("Video Frames Per Second: %.4f", n_vFps);
        n_vLastFpsTime = now;
        n_vLastFrameCount = n_vFrameCount;
    }
}

/*===========================================================================
 * FUNCTION   : debugShowPreviewFPS
 *
 * DESCRIPTION: helper function to log preview frame FPS for debug purpose.
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *==========================================================================*/
void QCamera2HardwareInterface::debugShowPreviewFPS()
{
    static int n_pFrameCount = 0;
    static int n_pLastFrameCount = 0;
    static nsecs_t n_pLastFpsTime = 0;
    static float n_pFps = 0;
    n_pFrameCount++;
    nsecs_t now = systemTime();
    nsecs_t diff = now - n_pLastFpsTime;
    if (diff > ms2ns(250)) {
        n_pFps =  ((n_pFrameCount - n_pLastFrameCount) * float(s2ns(1))) / diff;
        ALOGE("Preview Frames Per Second: %.4f", n_pFps);
        n_pLastFpsTime = now;
        n_pLastFrameCount = n_pFrameCount;
    }
}

/*===========================================================================
 * FUNCTION   : ~QCameraCbNotifier
 *
 * DESCRIPTION: Destructor for exiting the callback context.
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *==========================================================================*/
QCameraCbNotifier::~QCameraCbNotifier()
{
    mProcTh.exit();
}

/*===========================================================================
 * FUNCTION   : releaseNotifications
 *
 * DESCRIPTION: callback for releasing data stored in the callback queue.
 *
 * PARAMETERS :
 *   @data      : data to be released
 *   @user_data : context data
 *
 * RETURN     : None
 *==========================================================================*/
void QCameraCbNotifier::releaseNotifications(void *data, void *user_data)
{
    qcamera_callback_argm_t *arg = ( qcamera_callback_argm_t * ) data;

    if ( ( NULL != arg ) && ( NULL != user_data ) ) {
        if ( arg->release_cb ) {
            arg->release_cb(arg->user_data, arg->cookie);
        }
    }
}

/*===========================================================================
 * FUNCTION   : matchSnapshotNotifications
 *
 * DESCRIPTION: matches snapshot data callbacks
 *
 * PARAMETERS :
 *   @data      : data to match
 *   @user_data : context data
 *
 * RETURN     : bool match
 *              true - match found
 *              false- match not found
 *==========================================================================*/
bool QCameraCbNotifier::matchSnapshotNotifications(void *data,
                                                   void */*user_data*/)
{
    qcamera_callback_argm_t *arg = ( qcamera_callback_argm_t * ) data;
    if ( NULL != arg ) {
        if ( QCAMERA_DATA_SNAPSHOT_CALLBACK == arg->cb_type ) {
            return true;
        }
    }

    return false;
}

/*===========================================================================
 * FUNCTION   : cbNotifyRoutine
 *
 * DESCRIPTION: callback thread which interfaces with the upper layers
 *              given input commands.
 *
 * PARAMETERS :
 *   @data    : context data
 *
 * RETURN     : None
 *==========================================================================*/
void * QCameraCbNotifier::cbNotifyRoutine(void * data)
{
    int running = 1;
    int ret;
    QCameraCbNotifier *pme = (QCameraCbNotifier *)data;
    QCameraCmdThread *cmdThread = &pme->mProcTh;
    uint8_t isSnapshotActive = FALSE;
    uint32_t numOfSnapshotExpected = 0;
    uint32_t numOfSnapshotRcvd = 0;

    ALOGV("%s: E", __func__);
    do {
        do {
            ret = cam_sem_wait(&cmdThread->cmd_sem);
            if (ret != 0 && errno != EINVAL) {
                ALOGV("%s: cam_sem_wait error (%s)",
                           __func__, strerror(errno));
                return NULL;
            }
        } while (ret != 0);

        camera_cmd_type_t cmd = cmdThread->getCmd();
        ALOGV("%s: get cmd %d", __func__, cmd);
        switch (cmd) {
        case CAMERA_CMD_TYPE_START_DATA_PROC:
            {
                isSnapshotActive = TRUE;
                numOfSnapshotExpected = pme->mParent->numOfSnapshotsExpected();
                numOfSnapshotRcvd = 0;
            }
            break;
        case CAMERA_CMD_TYPE_STOP_DATA_PROC:
            {
                pme->mDataQ.flushNodes(matchSnapshotNotifications);
                isSnapshotActive = FALSE;

                numOfSnapshotExpected = 0;
                numOfSnapshotRcvd = 0;
            }
            break;
        case CAMERA_CMD_TYPE_DO_NEXT_JOB:
            {
                qcamera_callback_argm_t *cb =
                    (qcamera_callback_argm_t *)pme->mDataQ.dequeue();
                if (NULL != cb) {
                    ALOGV("%s: cb type %d received",
                          __func__,
                          cb->cb_type);

                    if (pme->mParent->msgTypeEnabledWithLock(cb->msg_type)) {
                        switch (cb->cb_type) {
                        case QCAMERA_NOTIFY_CALLBACK:
                            {
                                if (cb->msg_type == CAMERA_MSG_FOCUS) {
                                    ALOGD("[KPI Perf] %s : sending focus evt to app", __func__);
                                }
                                if (pme->mNotifyCb) {
                                    pme->mNotifyCb(cb->msg_type,
                                                  cb->ext1,
                                                  cb->ext2,
                                                  pme->mCallbackCookie);
                                } else {
                                    ALOGE("%s : notify callback not set!",
                                          __func__);
                                }
                            }
                            break;
                        case QCAMERA_DATA_CALLBACK:
                            {
                                if (pme->mDataCb) {
                                    pme->mDataCb(cb->msg_type,
                                                 cb->data,
                                                 cb->index,
                                                 cb->metadata,
                                                 pme->mCallbackCookie);
                                } else {
                                    ALOGE("%s : data callback not set!",
                                          __func__);
                                }
                            }
                            break;
                        case QCAMERA_DATA_TIMESTAMP_CALLBACK:
                            {
                                if(pme->mDataCbTimestamp) {
                                    pme->mDataCbTimestamp(cb->timestamp,
                                                          cb->msg_type,
                                                          cb->data,
                                                          cb->index,
                                                          pme->mCallbackCookie);
                                } else {
                                    ALOGE("%s:data cb with tmp not set!",
                                          __func__);
                                }
                            }
                            break;
                        case QCAMERA_DATA_SNAPSHOT_CALLBACK:
                            {
                                if (TRUE == isSnapshotActive && pme->mDataCb ) {
                                    numOfSnapshotRcvd++;
                                    if (numOfSnapshotExpected > 0 &&
                                        numOfSnapshotExpected == numOfSnapshotRcvd) {
                                        // notify HWI that snapshot is done
                                        pme->mParent->processSyncEvt(QCAMERA_SM_EVT_SNAPSHOT_DONE,
                                                                     NULL);
                                    }
                                    pme->mDataCb(cb->msg_type,
                                                 cb->data,
                                                 cb->index,
                                                 cb->metadata,
                                                 pme->mCallbackCookie);
                                }
                            }
                            break;
                        default:
                            {
                                ALOGE("%s : invalid cb type %d",
                                      __func__,
                                      cb->cb_type);
                            }
                            break;
                        };
                    } else {
                        ALOGE("%s : cb message type %d not enabled!",
                              __func__,
                              cb->msg_type);
                    }
                    if ( cb->release_cb ) {
                        cb->release_cb(cb->user_data, cb->cookie);
                    }
                    delete cb;
                } else {
                    ALOGE("%s: invalid cb type passed", __func__);
                }
            }
            break;
        case CAMERA_CMD_TYPE_EXIT:
            {
                pme->mDataQ.flush();
                running = 0;
            }
            break;
        default:
            break;
        }
    } while (running);
    ALOGV("%s: X", __func__);

    return NULL;
}

/*===========================================================================
 * FUNCTION   : notifyCallback
 *
 * DESCRIPTION: Enqueus pending callback notifications for the upper layers.
 *
 * PARAMETERS :
 *   @cbArgs  : callback arguments
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraCbNotifier::notifyCallback(qcamera_callback_argm_t &cbArgs)
{
    qcamera_callback_argm_t *cbArg = new qcamera_callback_argm_t();
    if (NULL == cbArg) {
        ALOGE("%s: no mem for qcamera_callback_argm_t", __func__);
        return NO_MEMORY;
    }
    memset(cbArg, 0, sizeof(qcamera_callback_argm_t));
    *cbArg = cbArgs;

    if (mDataQ.enqueue((void *)cbArg)) {
        mProcTh.sendCmd(CAMERA_CMD_TYPE_DO_NEXT_JOB, FALSE, FALSE);
    } else {
        ALOGE("%s: Error adding cb data into queue", __func__);
        delete cbArg;
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : setCallbacks
 *
 * DESCRIPTION: Initializes the callback functions, which would be used for
 *              communication with the upper layers and launches the callback
 *              context in which the callbacks will occur.
 *
 * PARAMETERS :
 *   @notifyCb          : notification callback
 *   @dataCb            : data callback
 *   @dataCbTimestamp   : data with timestamp callback
 *   @callbackCookie    : callback context data
 *
 * RETURN     : None
 *==========================================================================*/
void QCameraCbNotifier::setCallbacks(camera_notify_callback notifyCb,
                                     camera_data_callback dataCb,
                                     camera_data_timestamp_callback dataCbTimestamp,
                                     void *callbackCookie)
{
    if ( ( NULL == mNotifyCb ) &&
         ( NULL == mDataCb ) &&
         ( NULL == mDataCbTimestamp ) &&
         ( NULL == mCallbackCookie ) ) {
        mNotifyCb = notifyCb;
        mDataCb = dataCb;
        mDataCbTimestamp = dataCbTimestamp;
        mCallbackCookie = callbackCookie;
        mProcTh.launch(cbNotifyRoutine, this);
    } else {
        ALOGE("%s : Camera callback notifier already initialized!",
              __func__);
    }
}

/*===========================================================================
 * FUNCTION   : startSnapshots
 *
 * DESCRIPTION: Enables snapshot mode
 *
 * PARAMETERS : None
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraCbNotifier::startSnapshots()
{
    return mProcTh.sendCmd(CAMERA_CMD_TYPE_START_DATA_PROC, FALSE, TRUE);
}

/*===========================================================================
 * FUNCTION   : stopSnapshots
 *
 * DESCRIPTION: Disables snapshot processing mode
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *==========================================================================*/
void QCameraCbNotifier::stopSnapshots()
{
    mProcTh.sendCmd(CAMERA_CMD_TYPE_STOP_DATA_PROC, FALSE, TRUE);
}

}; // namespace qcamera
