/* Copyright (c) 2012-2014, The Linux Foundataion. All rights reserved.
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

#include <time.h>
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
    CDBG_HIGH("[KPI Perf] %s: E",__func__);
    char value[PROPERTY_VALUE_MAX];
    bool dump_raw = false;
    bool dump_yuv = false;
    bool log_matching = false;
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

    if(pme->mParameters.isSceneSelectionEnabled() &&
            !pme->m_stateMachine.isCaptureRunning()) {
        pme->selectScene(pChannel, recvd_frame);
        pChannel->bufDone(recvd_frame);
        return;
    }

    CDBG_HIGH("%s: [ZSL Retro] Frame CB Unlock : %d, is AEC Locked: %d",
          __func__, recvd_frame->bUnlockAEC, pme->m_bLedAfAecLock);
    if(recvd_frame->bUnlockAEC && pme->m_bLedAfAecLock) {
       ALOGI("%s : [ZSL Retro] LED assisted AF Release AEC Lock\n", __func__);
       pme->mParameters.setAecLock("false");
       pme->mParameters.commitParameters();
       pme->m_bLedAfAecLock = FALSE ;
    }

    // Check if retro-active frames are completed and camera is
    // ready to go ahead with LED estimation for regular frames
    if (recvd_frame->bReadyForPrepareSnapshot) {
      // Send an event
      CDBG_HIGH("%s: [ZSL Retro] Ready for Prepare Snapshot, signal ", __func__);
      qcamera_sm_internal_evt_payload_t *payload =
         (qcamera_sm_internal_evt_payload_t *)malloc(sizeof(qcamera_sm_internal_evt_payload_t));
      if (NULL != payload) {
        memset(payload, 0, sizeof(qcamera_sm_internal_evt_payload_t));
        payload->evt_type = QCAMERA_INTERNAL_EVT_READY_FOR_SNAPSHOT;
        int32_t rc = pme->processEvt(QCAMERA_SM_EVT_EVT_INTERNAL, payload);
        if (rc != NO_ERROR) {
          ALOGE("%s: processEvt Ready for Snaphot failed", __func__);
          free(payload);
          payload = NULL;
        }
      } else {
        ALOGE("%s: No memory for prepare signal event detect"
              " qcamera_sm_internal_evt_payload_t", __func__);
      }
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

    if (recvd_frame->num_bufs > 0) {
        ALOGI("[KPI Perf] %s: superbuf frame_idx %d", __func__,
            recvd_frame->bufs[0]->frame_idx);
    }

    // DUMP RAW if available
    property_get("persist.camera.zsl_raw", value, "0");
    dump_raw = atoi(value) > 0 ? true : false;
    if ( dump_raw ) {
        for ( int i= 0 ; i < recvd_frame->num_bufs ; i++ ) {
            if ( recvd_frame->bufs[i]->stream_type == CAM_STREAM_TYPE_RAW ) {
                mm_camera_buf_def_t * raw_frame = recvd_frame->bufs[i];
                QCameraStream *pStream = pChannel->getStreamByHandle(raw_frame->stream_id);
                if ( NULL != pStream ) {
                    pme->dumpFrameToFile(pStream, raw_frame, QCAMERA_DUMP_FRM_RAW);
                }
                break;
            }
        }
    }

    // DUMP YUV before reprocess if needed
    property_get("persist.camera.zsl_yuv", value, "0");
    dump_yuv = atoi(value) > 0 ? true : false;
    if ( dump_yuv ) {
        for ( int i= 0 ; i < recvd_frame->num_bufs ; i++ ) {
            if ( recvd_frame->bufs[i]->stream_type == CAM_STREAM_TYPE_SNAPSHOT ) {
                mm_camera_buf_def_t * yuv_frame = recvd_frame->bufs[i];
                QCameraStream *pStream = pChannel->getStreamByHandle(yuv_frame->stream_id);
                if ( NULL != pStream ) {
                    pme->dumpFrameToFile(pStream, yuv_frame, QCAMERA_DUMP_FRM_SNAPSHOT);
                }
                break;
            }
        }
    }
    //
    // whether need FD Metadata along with Snapshot frame in ZSL mode
    if(pme->needFDMetadata(QCAMERA_CH_TYPE_ZSL)){
        //Need Face Detection result for snapshot frames
        //Get the Meta Data frames
        mm_camera_buf_def_t *pMetaFrame = NULL;
        for(int i = 0; i < frame->num_bufs; i++){
            QCameraStream *pStream = pChannel->getStreamByHandle(frame->bufs[i]->stream_id);
            if(pStream != NULL){
                if(pStream->isTypeOf(CAM_STREAM_TYPE_METADATA)){
                    pMetaFrame = frame->bufs[i]; //find the metadata
                    break;
                }
            }
        }

        if(pMetaFrame != NULL){
            metadata_buffer_t *pMetaData = (metadata_buffer_t *)pMetaFrame->buffer;
            //send the face detection info
            uint8_t found = 0;
            cam_face_detection_data_t faces_data;
            if (IS_META_AVAILABLE(CAM_INTF_META_FACE_DETECTION, pMetaData)) {
                faces_data = *((cam_face_detection_data_t *)
                    POINTER_OF_META(CAM_INTF_META_FACE_DETECTION, pMetaData));
                found = 1;
            }
            faces_data.fd_type = QCAMERA_FD_SNAPSHOT; //HARD CODE here before MCT can support
            if(!found){
                faces_data.num_faces_detected = 0;
            }else if(faces_data.num_faces_detected > MAX_ROI){
                ALOGE("%s: Invalid number of faces %d",
                    __func__, faces_data.num_faces_detected);
            }
            qcamera_sm_internal_evt_payload_t *payload =
                (qcamera_sm_internal_evt_payload_t *)malloc(sizeof(qcamera_sm_internal_evt_payload_t));
            if (NULL != payload) {
                memset(payload, 0, sizeof(qcamera_sm_internal_evt_payload_t));
                payload->evt_type = QCAMERA_INTERNAL_EVT_FACE_DETECT_RESULT;
                payload->faces_data = faces_data;
                int32_t rc = pme->processEvt(QCAMERA_SM_EVT_EVT_INTERNAL, payload);
                if (rc != NO_ERROR) {
                    ALOGE("%s: processEvt face_detection_result failed", __func__);
                    free(payload);
                    payload = NULL;
                }
            } else {
                ALOGE("%s: No memory for face_detection_result qcamera_sm_internal_evt_payload_t", __func__);
            }
        }
    }

    property_get("persist.camera.dumpmetadata", value, "0");
    int32_t enabled = atoi(value);
    if (enabled) {
        mm_camera_buf_def_t *pMetaFrame = NULL;
        QCameraStream *pStream = NULL;
        for(int i = 0; i < frame->num_bufs; i++){
            pStream = pChannel->getStreamByHandle(frame->bufs[i]->stream_id);
            if(pStream != NULL){
                if(pStream->isTypeOf(CAM_STREAM_TYPE_METADATA)){
                    pMetaFrame = frame->bufs[i];
                    if(pMetaFrame != NULL &&
                       ((metadata_buffer_t *)pMetaFrame->buffer)->is_tuning_params_valid){
                        pme->dumpMetadataToFile(pStream, pMetaFrame,(char *)"ZSL_Snapshot");
                    }
                    break;
                }
            }
        }
    }

    property_get("persist.camera.zsl_matching", value, "0");
    log_matching = atoi(value) > 0 ? true : false;
    if (log_matching) {
        CDBG_HIGH("%s : ZSL super buffer contains:", __func__);
        QCameraStream *pStream = NULL;
        for (int i = 0; i < frame->num_bufs; i++) {
            pStream = pChannel->getStreamByHandle(frame->bufs[i]->stream_id);
            if (pStream != NULL ) {
                CDBG_HIGH("%s: Buffer with V4L index %d frame index %d of type %d Timestamp: %ld %ld ",
                        __func__,
                        frame->bufs[i]->buf_idx,
                        frame->bufs[i]->frame_idx,
                        pStream->getMyType(),
                        frame->bufs[i]->ts.tv_sec,
                        frame->bufs[i]->ts.tv_nsec);
            }
        }
    }

    // send to postprocessor
    pme->m_postprocessor.processData(frame);

    CDBG_HIGH("[KPI Perf] %s: X", __func__);
}

/*===========================================================================
 * FUNCTION   : selectScene
 *
 * DESCRIPTION: send a preview callback when a specific selected scene is applied
 *
 * PARAMETERS :
 *   @pChannel: Camera channel
 *   @frame   : Bundled super buffer
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera2HardwareInterface::selectScene(QCameraChannel *pChannel,
        mm_camera_super_buf_t *frame)
{
    mm_camera_buf_def_t *pMetaFrame = NULL;
    QCameraStream *pStream = NULL;
    cam_scene_mode_type *scene = NULL;
    cam_scene_mode_type selectedScene = CAM_SCENE_MODE_MAX;
    int32_t rc = NO_ERROR;

    if ((NULL == frame) || (NULL == pChannel)) {
        ALOGE("%s: Invalid scene select input", __func__);
        return BAD_VALUE;
    }

    selectedScene = mParameters.getSelectedScene();
    if (CAM_SCENE_MODE_MAX == selectedScene) {
        ALOGV("%s: No selected scene", __func__);
        return NO_ERROR;
    }

    for(int i = 0; i < frame->num_bufs; i++){
        pStream = pChannel->getStreamByHandle(frame->bufs[i]->stream_id);
        if(pStream != NULL){
            if(pStream->isTypeOf(CAM_STREAM_TYPE_METADATA)){
                pMetaFrame = frame->bufs[i];
                break;
            }
        }
    }

    if (NULL == pMetaFrame) {
        ALOGE("%s: No metadata buffer found in scene select super buffer", __func__);
        return NO_INIT;
    }

    metadata_buffer_t *pMetaData = (metadata_buffer_t *)pMetaFrame->buffer;
    if (IS_META_AVAILABLE(CAM_INTF_META_CURRENT_SCENE, pMetaData)) {
        scene = (cam_scene_mode_type *)
                POINTER_OF_META(CAM_INTF_META_CURRENT_SCENE, pMetaData);
    }

    if (NULL == scene) {
        ALOGE("%s: No current scene metadata!", __func__);
        return NO_INIT;
    }

    if ((*scene == selectedScene) &&
            (mDataCb != NULL) &&
            (msgTypeEnabledWithLock(CAMERA_MSG_PREVIEW_FRAME) > 0)) {
        mm_camera_buf_def_t *preview_frame = NULL;
        for(int i = 0; i < frame->num_bufs; i++){
            pStream = pChannel->getStreamByHandle(frame->bufs[i]->stream_id);
            if(pStream != NULL){
                if(pStream->isTypeOf(CAM_STREAM_TYPE_PREVIEW)){
                    preview_frame = frame->bufs[i];
                    break;
                }
            }
        }
        if (preview_frame) {
            QCameraGrallocMemory *memory = (QCameraGrallocMemory *)preview_frame->mem_info;
            int32_t idx = preview_frame->buf_idx;
            rc = sendPreviewCallback(pStream, memory, idx);
            if (NO_ERROR != rc) {
                ALOGE("%s: Error triggering scene select preview callback", __func__);
            } else {
                mParameters.setSelectedScene(CAM_SCENE_MODE_MAX);
            }
        } else {
            ALOGE("%s: No preview buffer found in scene select super buffer", __func__);
            return NO_INIT;
        }
    }

    return rc;
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
    char value[PROPERTY_VALUE_MAX];
    CDBG_HIGH("[KPI Perf] %s: E PROFILE_YUV_CB_TO_HAL", __func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != recvd_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
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

    property_get("persist.camera.dumpmetadata", value, "0");
    int32_t enabled = atoi(value);
    if (enabled) {
        mm_camera_buf_def_t *pMetaFrame = NULL;
        QCameraStream *pStream = NULL;
        for(int i = 0; i < frame->num_bufs; i++){
            pStream = pChannel->getStreamByHandle(frame->bufs[i]->stream_id);
            if(pStream != NULL){
                if(pStream->isTypeOf(CAM_STREAM_TYPE_METADATA)){
                    pMetaFrame = frame->bufs[i]; //find the metadata
                    if(pMetaFrame != NULL &&
                       ((metadata_buffer_t *)pMetaFrame->buffer)->is_tuning_params_valid){
                        pme->dumpMetadataToFile(pStream, pMetaFrame,(char *)"Snapshot");
                    }
                    break;
                }
            }
        }
    }

    // Wait on Postproc initialization if needed
    pme->waitDefferedWork(pme->mReprocJob);

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
                if (pStream->isTypeOf(CAM_STREAM_TYPE_SNAPSHOT)) {
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
            CDBG_HIGH("DEBUG: registerFaceImage E");
            int32_t rc = pme->registerFaceImage(main_frame->buffer, &config, faceId);
            CDBG_HIGH("DEBUG: registerFaceImage X, ret=%d, faceId=%d", rc, faceId);
            bRunFaceReg = 0;
        }
    }

#endif
/* END of test register face image for face authentication */

    CDBG_HIGH("[KPI Perf] %s: X", __func__);
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
    CDBG_HIGH("[KPI Perf] %s: E", __func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != recvd_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
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

    CDBG_HIGH("[KPI Perf] %s: X", __func__);
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
    CDBG_HIGH("[KPI Perf] %s : BEGIN", __func__);
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
    pme->dumpFrameToFile(stream, frame, QCAMERA_DUMP_FRM_PREVIEW);

    if(pme->m_bPreviewStarted) {
       CDBG_HIGH("[KPI Perf] %s : PROFILE_FIRST_PREVIEW_FRAME", __func__);
       pme->m_bPreviewStarted = false ;
    }

    // Display the buffer.
    CDBG("%p displayBuffer %d E", pme, idx);
    int dequeuedIdx = memory->displayBuffer(idx);
    if (dequeuedIdx < 0 || dequeuedIdx >= memory->getCnt()) {
        CDBG_HIGH("%s: Invalid dequeued buffer index %d from display",
              __func__, dequeuedIdx);
    } else {
        // Return dequeued buffer back to driver
        err = stream->bufDone(dequeuedIdx);
        if ( err < 0) {
            ALOGE("stream bufDone failed %d", err);
        }
    }

    // Handle preview data callback
    if (pme->mDataCb != NULL &&
            (pme->msgTypeEnabledWithLock(CAMERA_MSG_PREVIEW_FRAME) > 0) &&
            (!pme->mParameters.isSceneSelectionEnabled())) {
        int32_t rc = pme->sendPreviewCallback(stream, memory, idx);
        if (NO_ERROR != rc) {
            ALOGE("%s: Preview callback was not sent succesfully", __func__);
        }
    }

    free(super_frame);
    CDBG_HIGH("[KPI Perf] %s : END", __func__);
    return;
}

/*===========================================================================
 * FUNCTION   : sendPreviewCallback
 *
 * DESCRIPTION: helper function for triggering preview callbacks
 *
 * PARAMETERS :
 *   @stream    : stream object
 *   @memory    : Gralloc memory allocator
 *   @idx       : buffer index
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera2HardwareInterface::sendPreviewCallback(QCameraStream *stream,
        QCameraGrallocMemory *memory, int32_t idx)
{
    camera_memory_t *previewMem = NULL;
    camera_memory_t *data = NULL;
    int previewBufSize;
    cam_dimension_t preview_dim;
    cam_format_t previewFmt;
    int32_t rc = NO_ERROR;

    if ((NULL == stream) || (NULL == memory)) {
        ALOGE("%s: Invalid preview callback input", __func__);
        return BAD_VALUE;
    }

    stream->getFrameDimension(preview_dim);
    stream->getFormat(previewFmt);

    /* The preview buffer size in the callback should be
     * (width*height*bytes_per_pixel). As all preview formats we support,
     * use 12 bits per pixel, buffer size = previewWidth * previewHeight * 3/2.
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
            previewMem = mGetMemory(memory->getFd(idx),
                       previewBufSize, 1, mCallbackCookie);
            if (!previewMem || !previewMem->data) {
                ALOGE("%s: mGetMemory failed.\n", __func__);
                return NO_MEMORY;
            } else {
                data = previewMem;
            }
        } else
            data = memory->getMemory(idx, false);
    } else {
        data = memory->getMemory(idx, false);
        ALOGE("%s: Invalid preview format, buffer size in preview callback may be wrong.",
                __func__);
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
    cbArg.cookie = this;
    rc = m_cbNotifier.notifyCallback(cbArg);
    if (rc != NO_ERROR) {
        ALOGE("%s: fail sending notification", __func__);
        if (previewMem) {
            previewMem->release(previewMem);
        }
    }

    return rc;
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
    CDBG_HIGH("[KPI Perf] %s E",__func__);
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
        ALOGE("%s: preview frame is NULL", __func__);
        free(super_frame);
        return;
    }

    if (!pme->needProcessPreviewFrame()) {
        CDBG_HIGH("%s: preview is not running, no need to process", __func__);
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
        pme->dumpFrameToFile(stream, frame, QCAMERA_DUMP_FRM_PREVIEW);

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
            int32_t rc = pme->m_cbNotifier.notifyCallback(cbArg);
            if (rc != NO_ERROR) {
                ALOGE("%s: fail sending data notify", __func__);
                stream->bufDone(frame->buf_idx);
            }
        } else {
            stream->bufDone(frame->buf_idx);
        }
    }
    free(super_frame);
    CDBG_HIGH("[KPI Perf] %s X",__func__);
}

/*===========================================================================
 * FUNCTION   : rdi_mode_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle RDI frame from preview stream in
 *              rdi mode case
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
void QCamera2HardwareInterface::rdi_mode_stream_cb_routine(
  mm_camera_super_buf_t *super_frame,
  QCameraStream *stream,
  void * userdata)
{
    CDBG_HIGH("RDI_DEBUG %s[%d]: Enter", __func__, __LINE__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != super_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        free(super_frame);
        return;
    }
    mm_camera_buf_def_t *frame = super_frame->bufs[0];
    if (NULL == frame) {
        ALOGE("%s: preview frame is NLUL", __func__);
        goto end;
    }
    if (!pme->needProcessPreviewFrame()) {
        ALOGE("%s: preview is not running, no need to process", __func__);
        stream->bufDone(frame->buf_idx);
        goto end;
    }
    if (pme->needDebugFps()) {
        pme->debugShowPreviewFPS();
    }
    // Non-secure Mode
    if (!pme->isSecureMode()) {
        QCameraMemory *previewMemObj = (QCameraMemory *)frame->mem_info;
        if (NULL == previewMemObj) {
            ALOGE("%s: previewMemObj is NULL", __func__);
            stream->bufDone(frame->buf_idx);
            goto end;
        }

        camera_memory_t *preview_mem = previewMemObj->getMemory(frame->buf_idx, false);
        if (NULL != preview_mem) {
            previewMemObj->cleanCache(frame->buf_idx);
            // Dump RAW frame
            pme->dumpFrameToFile(stream, frame, QCAMERA_DUMP_FRM_RAW);
            // Notify Preview callback frame
            if (pme->needProcessPreviewFrame() &&
                    pme->mDataCb != NULL &&
                    pme->msgTypeEnabledWithLock(CAMERA_MSG_PREVIEW_FRAME) > 0) {
                qcamera_callback_argm_t cbArg;
                memset(&cbArg, 0, sizeof(qcamera_callback_argm_t));
                cbArg.cb_type    = QCAMERA_DATA_CALLBACK;
                cbArg.msg_type   = CAMERA_MSG_PREVIEW_FRAME;
                cbArg.data       = preview_mem;
                int user_data    = frame->buf_idx;
                cbArg.user_data  = (void *)user_data;
                cbArg.cookie     = stream;
                cbArg.release_cb = returnStreamBuffer;
                pme->m_cbNotifier.notifyCallback(cbArg);
            } else {
                ALOGE("%s: preview_mem is NULL", __func__);
                stream->bufDone(frame->buf_idx);
            }
        }
        else {
            ALOGE("%s: preview_mem is NULL", __func__);
            stream->bufDone(frame->buf_idx);
        }
    } else {
        // Secure Mode
        // We will do QCAMERA_NOTIFY_CALLBACK and share FD in case of secure mode
        QCameraMemory *previewMemObj = (QCameraMemory *)frame->mem_info;
        if (NULL == previewMemObj) {
            ALOGE("%s: previewMemObj is NULL", __func__);
            stream->bufDone(frame->buf_idx);
            goto end;
        }

        int fd = previewMemObj->getFd(frame->buf_idx);
        ALOGD("%s: Preview frame fd =%d for index = %d ", __func__, fd, frame->buf_idx);
        if (pme->needProcessPreviewFrame() &&
                pme->mDataCb != NULL &&
                pme->msgTypeEnabledWithLock(CAMERA_MSG_PREVIEW_FRAME) > 0) {
            // Prepare Callback structure
            qcamera_callback_argm_t cbArg;
            memset(&cbArg, 0, sizeof(qcamera_callback_argm_t));
            cbArg.cb_type    = QCAMERA_NOTIFY_CALLBACK;
            cbArg.msg_type   = CAMERA_MSG_PREVIEW_FRAME;
#ifndef VANILLA_HAL
            cbArg.ext1       = CAMERA_FRAME_DATA_FD;
            cbArg.ext2       = fd;
#endif
            int user_data    = frame->buf_idx;
            cbArg.user_data  = (void *)user_data;
            cbArg.cookie     = stream;
            cbArg.release_cb = returnStreamBuffer;
            pme->m_cbNotifier.notifyCallback(cbArg);
        } else {
            CDBG_HIGH("%s: No need to process preview frame, return buffer", __func__);
            stream->bufDone(frame->buf_idx);
        }
    }
end:
    free(super_frame);
    CDBG_HIGH("RDI_DEBUG %s[%d]: Exit", __func__, __LINE__);
    return;
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

    CDBG_HIGH("[KPI Perf] %s : BEGIN", __func__);

    mm_camera_buf_def_t *frame = super_frame->bufs[0];
    if (NULL == frame) {
        ALOGE("%s: preview frame is NULL", __func__);
        free(super_frame);
        return;
    }

    QCameraMemory *memObj = (QCameraMemory *)frame->mem_info;
    if (NULL != memObj) {
        pme->dumpFrameToFile(stream, frame, QCAMERA_DUMP_FRM_THUMBNAIL);
    }

    // Return buffer back to driver
    err = stream->bufDone(frame->buf_idx);
    if ( err < 0) {
        ALOGE("stream bufDone failed %d", err);
    }

    free(super_frame);
    CDBG_HIGH("[KPI Perf] %s : END", __func__);
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
                                                        QCameraStream *stream,
                                                        void *userdata)
{
    CDBG_HIGH("[KPI Perf] %s : BEGIN", __func__);
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
    if(pme->m_bRecordStarted) {
       CDBG_HIGH("[KPI Perf] %s : PROFILE_FIRST_RECORD_FRAME", __func__);
       pme->m_bRecordStarted = false ;
    }
    CDBG_HIGH("%s: Stream(%d), Timestamp: %ld %ld",
          __func__,
          frame->stream_id,
          frame->ts.tv_sec,
          frame->ts.tv_nsec);
    nsecs_t timeStamp;
    if(pme->mParameters.isAVTimerEnabled() == true) {
        timeStamp = (nsecs_t)((frame->ts.tv_sec * 1000000LL) + frame->ts.tv_nsec) * 1000;
    } else {
        timeStamp = nsecs_t(frame->ts.tv_sec) * 1000000000LL + frame->ts.tv_nsec;
    }
    CDBG_HIGH("Send Video frame to services/encoder TimeStamp : %lld",
        timeStamp);
    QCameraMemory *videoMemObj = (QCameraMemory *)frame->mem_info;
    camera_memory_t *video_mem = NULL;
    if (NULL != videoMemObj) {
        video_mem = videoMemObj->getMemory(frame->buf_idx, (pme->mStoreMetaDataInFrame > 0)? true : false);
    }
    if (NULL != videoMemObj && NULL != video_mem) {
        pme->dumpFrameToFile(stream, frame, QCAMERA_DUMP_FRM_VIDEO);
        if ((pme->mDataCbTimestamp != NULL) &&
            pme->msgTypeEnabledWithLock(CAMERA_MSG_VIDEO_FRAME) > 0) {
            qcamera_callback_argm_t cbArg;
            memset(&cbArg, 0, sizeof(qcamera_callback_argm_t));
            cbArg.cb_type = QCAMERA_DATA_TIMESTAMP_CALLBACK;
            cbArg.msg_type = CAMERA_MSG_VIDEO_FRAME;
            cbArg.data = video_mem;
            cbArg.timestamp = timeStamp;
            int32_t rc = pme->m_cbNotifier.notifyCallback(cbArg);
            if (rc != NO_ERROR) {
                ALOGE("%s: fail sending data notify", __func__);
                stream->bufDone(frame->buf_idx);
            }
        }
    }
    free(super_frame);
    CDBG_HIGH("[KPI Perf] %s : END", __func__);
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
    char value[PROPERTY_VALUE_MAX];

    CDBG_HIGH("[KPI Perf] %s: E", __func__);
    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != super_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(super_frame);
        return;
    }

    property_get("persist.camera.dumpmetadata", value, "0");
    int32_t enabled = atoi(value);
    if (enabled) {
        QCameraChannel *pChannel = pme->m_channels[QCAMERA_CH_TYPE_SNAPSHOT];
        if (pChannel == NULL ||
            pChannel->getMyHandle() != super_frame->ch_id) {
            ALOGE("%s: Capture channel doesn't exist, return here", __func__);
            return;
        }
        mm_camera_buf_def_t *pMetaFrame = NULL;
        QCameraStream *pStream = NULL;
        for(int i = 0; i < super_frame->num_bufs; i++){
            pStream = pChannel->getStreamByHandle(super_frame->bufs[i]->stream_id);
            if(pStream != NULL){
                if(pStream->isTypeOf(CAM_STREAM_TYPE_METADATA)){
                    pMetaFrame = super_frame->bufs[i]; //find the metadata
                    if(pMetaFrame != NULL &&
                       ((metadata_buffer_t *)pMetaFrame->buffer)->is_tuning_params_valid){
                        pme->dumpMetadataToFile(pStream, pMetaFrame,(char *)"Snapshot");
                    }
                    break;
                }
            }
        }
    }

    pme->m_postprocessor.processData(super_frame);

    CDBG_HIGH("[KPI Perf] %s: X", __func__);
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
    CDBG_HIGH("[KPI Perf] %s : BEGIN", __func__);
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
    CDBG_HIGH("[KPI Perf] %s : END", __func__);
}

/*===========================================================================
 * FUNCTION   : preview_raw_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle raw frame during standard preview
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
void QCamera2HardwareInterface::preview_raw_stream_cb_routine(mm_camera_super_buf_t * super_frame,
                                                              QCameraStream * stream,
                                                              void * userdata)
{
    CDBG_HIGH("[KPI Perf] %s : BEGIN", __func__);
    int i = -1;
    char value[PROPERTY_VALUE_MAX];
    bool dump_raw = false;

    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != super_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(super_frame);
        return;
    }

    property_get("persist.camera.preview_raw", value, "0");
    dump_raw = atoi(value) > 0 ? true : false;

    for ( i= 0 ; i < super_frame->num_bufs ; i++ ) {
        if ( super_frame->bufs[i]->stream_type == CAM_STREAM_TYPE_RAW ) {
            mm_camera_buf_def_t * raw_frame = super_frame->bufs[i];
            if ( NULL != stream && (dump_raw) ) {
                pme->dumpFrameToFile(stream, raw_frame, QCAMERA_DUMP_FRM_RAW);
            }
            stream->bufDone(super_frame->bufs[i]->buf_idx);
            break;
        }
    }

    free(super_frame);

    CDBG_HIGH("[KPI Perf] %s : END", __func__);
}

/*===========================================================================
 * FUNCTION   : snapshot_raw_stream_cb_routine
 *
 * DESCRIPTION: helper function to handle raw frame during standard capture
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
void QCamera2HardwareInterface::snapshot_raw_stream_cb_routine(mm_camera_super_buf_t * super_frame,
                                                               QCameraStream * stream,
                                                               void * userdata)
{
    CDBG_HIGH("[KPI Perf] %s : BEGIN", __func__);
    int i = -1;
    char value[PROPERTY_VALUE_MAX];
    bool dump_raw = false;

    QCamera2HardwareInterface *pme = (QCamera2HardwareInterface *)userdata;
    if (pme == NULL ||
        pme->mCameraHandle == NULL ||
        pme->mCameraHandle->camera_handle != super_frame->camera_handle){
        ALOGE("%s: camera obj not valid", __func__);
        // simply free super frame
        free(super_frame);
        return;
    }

    property_get("persist.camera.snapshot_raw", value, "0");
    dump_raw = atoi(value) > 0 ? true : false;

    for ( i= 0 ; i < super_frame->num_bufs ; i++ ) {
        if ( super_frame->bufs[i]->stream_type == CAM_STREAM_TYPE_RAW ) {
            mm_camera_buf_def_t * raw_frame = super_frame->bufs[i];
            if ( NULL != stream && (dump_raw) ) {
                pme->dumpFrameToFile(stream, raw_frame, QCAMERA_DUMP_FRM_RAW);
            }
            stream->bufDone(super_frame->bufs[i]->buf_idx);
            break;
        }
    }

    free(super_frame);

    CDBG_HIGH("[KPI Perf] %s : END", __func__);
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
    CDBG("[KPI Perf] %s : BEGIN", __func__);
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
    metadata_buffer_t *pMetaData = (metadata_buffer_t *)frame->buffer;
    if(pme->m_stateMachine.isNonZSLCaptureRunning()&&
       !pme->mLongshotEnabled) {
       //Make shutter call back in non ZSL mode once raw frame is received from VFE.
       pme->playShutter();
    }

    if (pMetaData->is_tuning_params_valid && pme->mParameters.getRecordingHintValue() == true) {
        //Dump Tuning data for video
        pme->dumpMetadataToFile(stream,frame,(char *)"Video");
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_HISTOGRAM, pMetaData)) {
        cam_hist_stats_t *stats_data = (cam_hist_stats_t *)
            POINTER_OF_META(CAM_INTF_META_HISTOGRAM, pMetaData);
        // process histogram statistics info
        qcamera_sm_internal_evt_payload_t *payload =
            (qcamera_sm_internal_evt_payload_t *)
                malloc(sizeof(qcamera_sm_internal_evt_payload_t));
        if (NULL != payload) {
            memset(payload, 0, sizeof(qcamera_sm_internal_evt_payload_t));
            payload->evt_type = QCAMERA_INTERNAL_EVT_HISTOGRAM_STATS;
            payload->stats_data = *stats_data;
            int32_t rc = pme->processEvt(QCAMERA_SM_EVT_EVT_INTERNAL, payload);
            if (rc != NO_ERROR) {
                ALOGE("%s: processEvt histogram failed", __func__);
                free(payload);
                payload = NULL;

            }
        } else {
            ALOGE("%s: No memory for histogram qcamera_sm_internal_evt_payload_t", __func__);
        }
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_FACE_DETECTION, pMetaData)) {
        cam_face_detection_data_t *faces_data = (cam_face_detection_data_t *)
            POINTER_OF_META(CAM_INTF_META_FACE_DETECTION, pMetaData);
        if (faces_data->num_faces_detected > MAX_ROI) {
            ALOGE("%s: Invalid number of faces %d",
                __func__, faces_data->num_faces_detected);
        } else {
            // process face detection result
            if (faces_data->num_faces_detected)
                ALOGI("[KPI Perf] %s: PROFILE_NUMBER_OF_FACES_DETECTED %d",
                    __func__,faces_data->num_faces_detected);
            faces_data->fd_type = QCAMERA_FD_PREVIEW; //HARD CODE here before MCT can support
            qcamera_sm_internal_evt_payload_t *payload = (qcamera_sm_internal_evt_payload_t *)
                malloc(sizeof(qcamera_sm_internal_evt_payload_t));
            if (NULL != payload) {
                memset(payload, 0, sizeof(qcamera_sm_internal_evt_payload_t));
                payload->evt_type = QCAMERA_INTERNAL_EVT_FACE_DETECT_RESULT;
                payload->faces_data = *faces_data;
                int32_t rc = pme->processEvt(QCAMERA_SM_EVT_EVT_INTERNAL, payload);
                if (rc != NO_ERROR) {
                    ALOGE("%s: processEvt face detection failed", __func__);
                    free(payload);
                    payload = NULL;
                }
            } else {
                ALOGE("%s: No memory for face detect qcamera_sm_internal_evt_payload_t", __func__);
            }
        }
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_AUTOFOCUS_DATA, pMetaData)) {
        cam_auto_focus_data_t *focus_data = (cam_auto_focus_data_t *)
            POINTER_OF_META(CAM_INTF_META_AUTOFOCUS_DATA, pMetaData);
        qcamera_sm_internal_evt_payload_t *payload =
            (qcamera_sm_internal_evt_payload_t *)malloc(sizeof(qcamera_sm_internal_evt_payload_t));
        if (NULL != payload) {
            memset(payload, 0, sizeof(qcamera_sm_internal_evt_payload_t));
            payload->evt_type = QCAMERA_INTERNAL_EVT_FOCUS_UPDATE;
            payload->focus_data = *focus_data;
            int32_t rc = pme->processEvt(QCAMERA_SM_EVT_EVT_INTERNAL, payload);
            if (rc != NO_ERROR) {
                ALOGE("%s: processEvt focus failed", __func__);
                free(payload);
                payload = NULL;

            }
        } else {
            ALOGE("%s: No memory for focus qcamera_sm_internal_evt_payload_t", __func__);
        }
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_CROP_DATA, pMetaData)) {
        cam_crop_data_t *crop_data =
            (cam_crop_data_t *)POINTER_OF_META(CAM_INTF_META_CROP_DATA, pMetaData);
        if (crop_data->num_of_streams > MAX_NUM_STREAMS) {
            ALOGE("%s: Invalid num_of_streams %d in crop_data", __func__,
                crop_data->num_of_streams);
        } else {
            qcamera_sm_internal_evt_payload_t *payload =
                (qcamera_sm_internal_evt_payload_t *)
                    malloc(sizeof(qcamera_sm_internal_evt_payload_t));
            if (NULL != payload) {
                memset(payload, 0, sizeof(qcamera_sm_internal_evt_payload_t));
                payload->evt_type = QCAMERA_INTERNAL_EVT_CROP_INFO;
                payload->crop_data = *crop_data;
                int32_t rc = pme->processEvt(QCAMERA_SM_EVT_EVT_INTERNAL, payload);
                if (rc != NO_ERROR) {
                    ALOGE("%s: processEvt crop info failed", __func__);
                    free(payload);
                    payload = NULL;

                }
            } else {
                ALOGE("%s: No memory for prep_snapshot qcamera_sm_internal_evt_payload_t",
                    __func__);
            }
        }
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_PREP_SNAPSHOT_DONE, pMetaData)) {
        int32_t *prep_snapshot_done_state =
            (int32_t *)POINTER_OF_META(CAM_INTF_META_PREP_SNAPSHOT_DONE, pMetaData);
        qcamera_sm_internal_evt_payload_t *payload =
        (qcamera_sm_internal_evt_payload_t *)malloc(sizeof(qcamera_sm_internal_evt_payload_t));
        if (NULL != payload) {
            memset(payload, 0, sizeof(qcamera_sm_internal_evt_payload_t));
            payload->evt_type = QCAMERA_INTERNAL_EVT_PREP_SNAPSHOT_DONE;
            payload->prep_snapshot_state = (cam_prep_snapshot_state_t)*prep_snapshot_done_state;
            int32_t rc = pme->processEvt(QCAMERA_SM_EVT_EVT_INTERNAL, payload);
            if (rc != NO_ERROR) {
                ALOGE("%s: processEvt prep_snapshot failed", __func__);
                free(payload);
                payload = NULL;

            }
        } else {
            ALOGE("%s: No memory for prep_snapshot qcamera_sm_internal_evt_payload_t", __func__);
        }
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_ASD_HDR_SCENE_DATA, pMetaData)) {
        cam_asd_hdr_scene_data_t *hdr_scene_data =
        (cam_asd_hdr_scene_data_t *)POINTER_OF_META(CAM_INTF_META_ASD_HDR_SCENE_DATA, pMetaData);
        CDBG_HIGH("%s: hdr_scene_data: %d %f\n", __func__,
            hdr_scene_data->is_hdr_scene, hdr_scene_data->hdr_confidence);
        //Handle this HDR meta data only if capture is not in process
        if (!pme->m_stateMachine.isCaptureRunning()) {
            int32_t rc = pme->processHDRData(*hdr_scene_data);
            if (rc != NO_ERROR) {
                ALOGE("%s: processHDRData failed", __func__);
            }
        }
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_ASD_SCENE_TYPE, pMetaData)) {
        int32_t *scene =
            (int32_t *)POINTER_OF_META(CAM_INTF_META_ASD_SCENE_TYPE, pMetaData);
        qcamera_sm_internal_evt_payload_t *payload =
            (qcamera_sm_internal_evt_payload_t *)malloc(sizeof(qcamera_sm_internal_evt_payload_t));
        if (NULL != payload) {
            memset(payload, 0, sizeof(qcamera_sm_internal_evt_payload_t));
            payload->evt_type = QCAMERA_INTERNAL_EVT_ASD_UPDATE;
            payload->asd_data = (cam_auto_scene_t)*scene;
            int32_t rc = pme->processEvt(QCAMERA_SM_EVT_EVT_INTERNAL, payload);
            if (rc != NO_ERROR) {
                ALOGE("%s: processEvt asd_update failed", __func__);
                free(payload);
                payload = NULL;

            }
        } else {
            ALOGE("%s: No memory for asd_update qcamera_sm_internal_evt_payload_t", __func__);
        }
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_FLASH_MODE, pMetaData)) {
        uint8_t *flash_mode =
            (uint8_t *)POINTER_OF_META(CAM_INTF_META_FLASH_MODE, pMetaData);
        pme->mExifParams.sensor_params.flash_mode = (cam_flash_mode_t)*flash_mode;
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_FLASH_STATE, pMetaData)) {
        int32_t *flash_state =
            (int32_t *)POINTER_OF_META(CAM_INTF_META_FLASH_STATE, pMetaData);
        pme->mExifParams.sensor_params.flash_state = (cam_flash_state_t)*flash_state;
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_LENS_APERTURE, pMetaData)) {
        float *aperture_value =
            (float *)POINTER_OF_META(CAM_INTF_META_LENS_APERTURE, pMetaData);
        pme->mExifParams.sensor_params.aperture_value = *aperture_value;
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_AEC_INFO, pMetaData)) {
        cam_3a_params_t* ae_params =
            (cam_3a_params_t*)POINTER_OF_META(CAM_INTF_META_AEC_INFO, pMetaData);
        pme->mExifParams.cam_3a_params = *ae_params;
        pme->mFlashNeeded = ae_params->flash_needed;
    }
    if (IS_META_AVAILABLE(CAM_INTF_META_SENSOR_INFO, pMetaData)) {
        cam_sensor_params_t* sensor_params = (cam_sensor_params_t*)
            POINTER_OF_META(CAM_INTF_META_SENSOR_INFO, pMetaData);
        pme->mExifParams.sensor_params = *sensor_params;
    }

    stream->bufDone(frame->buf_idx);
    free(super_frame);

    CDBG("[KPI Perf] %s : END", __func__);
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
    CDBG_HIGH("[KPI Perf] %s: E", __func__);
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

    CDBG_HIGH("[KPI Perf] %s: X", __func__);
}

/*===========================================================================
 * FUNCTION   : dumpFrameToFile
 *
 * DESCRIPTION: helper function to dump jpeg into file for debug purpose.
 *
 * PARAMETERS :
 *    @data : data ptr
 *    @size : length of data buffer
 *    @index : identifier for data
 *
 * RETURN     : None
 *==========================================================================*/
void QCamera2HardwareInterface::dumpJpegToFile(const void *data,
                                               uint32_t size,
                                               int index)
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

    if((enabled & QCAMERA_DUMP_FRM_JPEG) && data) {
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
                snprintf(buf, sizeof(buf), "/data/%d_%d.jpg", mDumpFrmCnt, index);

                int file_fd = open(buf, O_RDWR | O_CREAT, 0777);
                if (file_fd > 0) {
                    int written_len = write(file_fd, data, size);
                    CDBG_HIGH("%s: written number of bytes %d\n",
                        __func__, written_len);
                    close(file_fd);
                } else {
                    ALOGE("%s: fail t open file for image dumping", __func__);
                }
                mDumpFrmCnt++;
            }
        }
        mDumpSkipCnt++;
    }
}


void QCamera2HardwareInterface::dumpMetadataToFile(QCameraStream *stream,
                                                   mm_camera_buf_def_t *frame,char *type)
{
    char value[PROPERTY_VALUE_MAX];
    int frm_num = 0;
    metadata_buffer_t *metadata = (metadata_buffer_t *)frame->buffer;
    property_get("persist.camera.dumpmetadata", value, "0");
    int32_t enabled = atoi(value);
    if (stream == NULL) {
        CDBG_HIGH("No op");
        return;
    }

    int mDumpFrmCnt = stream->mDumpMetaFrame;
    if(enabled){
        frm_num = ((enabled & 0xffff0000) >> 16);
        if(frm_num == 0) {
            frm_num = 10; //default 10 frames
        }
        if(frm_num > 256) {
            frm_num = 256; //256 buffers cycle around
        }
        if((frm_num == 256) && (mDumpFrmCnt >= frm_num)) {
            // reset frame count if cycling
            mDumpFrmCnt = 0;
        }
        CDBG_HIGH("mDumpFrmCnt= %d, frm_num = %d",mDumpFrmCnt,frm_num);
        if (mDumpFrmCnt >= 0 && mDumpFrmCnt < frm_num) {
            char timeBuf[128];
            char buf[32];
            memset(buf, 0, sizeof(buf));
            memset(timeBuf, 0, sizeof(timeBuf));
            time_t current_time;
            struct tm * timeinfo;
            time (&current_time);
            timeinfo = localtime (&current_time);
            strftime (timeBuf, sizeof(timeBuf),"/data/%Y%m%d%H%M%S", timeinfo);
            String8 filePath(timeBuf);
            snprintf(buf, sizeof(buf), "%dm_%s_%d.bin",
                                         mDumpFrmCnt,type,frame->frame_idx);
            filePath.append(buf);
            int file_fd = open(filePath.string(), O_RDWR | O_CREAT, 0777);
            if (file_fd > 0) {
                int written_len = 0;
                metadata->tuning_params.tuning_data_version = TUNING_DATA_VERSION;
                void *data = (void *)((uint8_t *)&metadata->tuning_params.tuning_data_version);
                written_len += write(file_fd, data, sizeof(uint32_t));
                data = (void *)((uint8_t *)&metadata->tuning_params.tuning_sensor_data_size);
                CDBG_HIGH("tuning_sensor_data_size %d",(int)(*(int *)data));
                written_len += write(file_fd, data, sizeof(uint32_t));
                data = (void *)((uint8_t *)&metadata->tuning_params.tuning_vfe_data_size);
                CDBG_HIGH("tuning_vfe_data_size %d",(int)(*(int *)data));
                written_len += write(file_fd, data, sizeof(uint32_t));
                data = (void *)((uint8_t *)&metadata->tuning_params.tuning_cpp_data_size);
                CDBG_HIGH("tuning_cpp_data_size %d",(int)(*(int *)data));
                written_len += write(file_fd, data, sizeof(uint32_t));
                data = (void *)((uint8_t *)&metadata->tuning_params.tuning_cac_data_size);
                CDBG_HIGH("tuning_cac_data_size %d",(int)(*(int *)data));
                written_len += write(file_fd, data, sizeof(uint32_t));
                int total_size = metadata->tuning_params.tuning_sensor_data_size;
                data = (void *)((uint8_t *)&metadata->tuning_params.data);
                written_len += write(file_fd, data, total_size);
                total_size = metadata->tuning_params.tuning_vfe_data_size;
                data = (void *)((uint8_t *)&metadata->tuning_params.data[TUNING_VFE_DATA_OFFSET]);
                written_len += write(file_fd, data, total_size);
                total_size = metadata->tuning_params.tuning_cpp_data_size;
                data = (void *)((uint8_t *)&metadata->tuning_params.data[TUNING_CPP_DATA_OFFSET]);
                written_len += write(file_fd, data, total_size);
                total_size = metadata->tuning_params.tuning_cac_data_size;
                data = (void *)((uint8_t *)&metadata->tuning_params.data[TUNING_CAC_DATA_OFFSET]);
                written_len += write(file_fd, data, total_size);
                close(file_fd);
            }else {
                ALOGE("%s: fail t open file for image dumping", __func__);
            }
            mDumpFrmCnt++;
        }
    }
    stream->mDumpMetaFrame = mDumpFrmCnt;
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
void QCamera2HardwareInterface::dumpFrameToFile(QCameraStream *stream,
                                                mm_camera_buf_def_t *frame,
                                                int dump_type)
{
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.camera.dumpimg", value, "0");
    int32_t enabled = atoi(value);
    int frm_num = 0;
    uint32_t skip_mode = 0;
    int mDumpFrmCnt = stream->mDumpFrame;

    if(enabled & QCAMERA_DUMP_FRM_MASK_ALL) {
        if((enabled & dump_type) && stream && frame) {
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
            if(stream->mDumpSkipCnt == 0)
                stream->mDumpSkipCnt = 1;

            if( stream->mDumpSkipCnt % skip_mode == 0) {
                if((frm_num == 256) && (mDumpFrmCnt >= frm_num)) {
                    // reset frame count if cycling
                    mDumpFrmCnt = 0;
                }
                if (mDumpFrmCnt >= 0 && mDumpFrmCnt <= frm_num) {
                    char buf[32];
                    char timeBuf[128];
                    time_t current_time;
                    struct tm * timeinfo;


                    time (&current_time);
                    timeinfo = localtime (&current_time);
                    memset(buf, 0, sizeof(buf));

                    cam_dimension_t dim;
                    memset(&dim, 0, sizeof(dim));
                    stream->getFrameDimension(dim);

                    cam_frame_len_offset_t offset;
                    memset(&offset, 0, sizeof(cam_frame_len_offset_t));
                    stream->getFrameOffset(offset);

                    strftime (timeBuf, sizeof(timeBuf),"/data/%Y%m%d%H%M%S", timeinfo);
                    String8 filePath(timeBuf);
                    switch (dump_type) {
                    case QCAMERA_DUMP_FRM_PREVIEW:
                        {
                            snprintf(buf, sizeof(buf), "%dp_%dx%d_%d.yuv",
                                     mDumpFrmCnt, dim.width, dim.height, frame->frame_idx);
                        }
                        break;
                    case QCAMERA_DUMP_FRM_THUMBNAIL:
                        {
                            snprintf(buf, sizeof(buf), "%dt_%dx%d_%d.yuv",
                                     mDumpFrmCnt, dim.width, dim.height, frame->frame_idx);
                        }
                        break;
                    case QCAMERA_DUMP_FRM_SNAPSHOT:
                        {
                            mParameters.getStreamDimension(CAM_STREAM_TYPE_SNAPSHOT, dim);
                            snprintf(buf, sizeof(buf), "%ds_%dx%d_%d.yuv",
                                    mDumpFrmCnt,
                                    dim.width,
                                    dim.height,
                                    frame->frame_idx);
                        }
                        break;
                    case QCAMERA_DUMP_FRM_VIDEO:
                        {
                            snprintf(buf, sizeof(buf), "%dv_%dx%d_%d.yuv",
                                     mDumpFrmCnt, dim.width, dim.height, frame->frame_idx);
                        }
                        break;
                    case QCAMERA_DUMP_FRM_RAW:
                        {
                            mParameters.getStreamDimension(CAM_STREAM_TYPE_RAW, dim);
                            snprintf(buf, sizeof(buf), "%dr_%dx%d_%d.raw",
                                    mDumpFrmCnt,
                                    dim.width,
                                    dim.height,
                                    frame->frame_idx);
                        }
                        break;
                    case QCAMERA_DUMP_FRM_JPEG:
                        {
                            mParameters.getStreamDimension(CAM_STREAM_TYPE_SNAPSHOT, dim);
                            snprintf(buf, sizeof(buf), "%dj_%dx%d_%d.yuv",
                                    mDumpFrmCnt,
                                    dim.width,
                                    dim.height,
                                    frame->frame_idx);
                        }
                        break;
                    default:
                        ALOGE("%s: Not supported for dumping stream type %d",
                              __func__, dump_type);
                        return;
                    }

                    filePath.append(buf);
                    int file_fd = open(filePath.string(), O_RDWR | O_CREAT, 0777);
                    if (file_fd > 0) {
                        void *data = NULL;
                        int written_len = 0;

                        for (int i = 0; i < offset.num_planes; i++) {
                            uint32_t index = offset.mp[i].offset;
                            if (i > 0) {
                                index += offset.mp[i-1].len;
                            }
                            for (int j = 0; j < offset.mp[i].height; j++) {
                                data = (void *)((uint8_t *)frame->buffer + index);
                                written_len += write(file_fd, data, offset.mp[i].width);
                                index += offset.mp[i].stride;
                            }
                        }

                        CDBG_HIGH("%s: written number of bytes %d\n",
                            __func__, written_len);
                        close(file_fd);
                    } else {
                        ALOGE("%s: fail t open file for image dumping", __func__);
                    }
                    mDumpFrmCnt++;
                }
            }
            stream->mDumpSkipCnt++;
        }
    } else {
        mDumpFrmCnt = 0;
    }
    stream->mDumpFrame = mDumpFrmCnt;
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
        CDBG_HIGH("Video Frames Per Second: %.4f", n_vFps);
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
        CDBG_HIGH("[KPI Perf] %s: PROFILE_PREVIEW_FRAMES_PER_SECOND : %.4f",
            __func__, n_pFps);
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
}

/*===========================================================================
 * FUNCTION   : exit
 *
 * DESCRIPTION: exit notify thread.
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *==========================================================================*/
void QCameraCbNotifier::exit()
{
    mActive = false;
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
            arg->release_cb(arg->user_data, arg->cookie, FAILED_TRANSACTION);
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
    bool longShotEnabled = false;
    uint32_t numOfSnapshotExpected = 0;
    uint32_t numOfSnapshotRcvd = 0;
    int32_t cbStatus = NO_ERROR;

    CDBG("%s: E", __func__);
    do {
        do {
            ret = cam_sem_wait(&cmdThread->cmd_sem);
            if (ret != 0 && errno != EINVAL) {
                CDBG("%s: cam_sem_wait error (%s)",
                           __func__, strerror(errno));
                return NULL;
            }
        } while (ret != 0);

        camera_cmd_type_t cmd = cmdThread->getCmd();
        CDBG("%s: get cmd %d", __func__, cmd);
        switch (cmd) {
        case CAMERA_CMD_TYPE_START_DATA_PROC:
            {
                isSnapshotActive = TRUE;
                numOfSnapshotExpected = pme->mParent->numOfSnapshotsExpected();
                longShotEnabled = pme->mParent->isLongshotEnabled();
                CDBG_HIGH("%s: Num Snapshots Expected = %d",
                  __func__, numOfSnapshotExpected);
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
                cbStatus = NO_ERROR;
                if (NULL != cb) {
                    CDBG("%s: cb type %d received",
                          __func__,
                          cb->cb_type);

                    if (pme->mParent->msgTypeEnabledWithLock(cb->msg_type)) {
                        switch (cb->cb_type) {
                        case QCAMERA_NOTIFY_CALLBACK:
                            {
                                if (cb->msg_type == CAMERA_MSG_FOCUS) {
                                    CDBG_HIGH("[KPI Perf] %s : PROFILE_SENDING_FOCUS_EVT_TO APP",
                                        __func__);
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
                                    if (!longShotEnabled) {
                                        numOfSnapshotRcvd++;
                                        CDBG_HIGH("%s: [ZSL Retro] Num Snapshots Received = %d", __func__,
                                                numOfSnapshotRcvd);
                                        if (numOfSnapshotExpected > 0 &&
                                           (numOfSnapshotExpected == numOfSnapshotRcvd)) {
                                            CDBG_HIGH("%s: [ZSL Retro] Expected snapshot received = %d",
                                                    __func__, numOfSnapshotRcvd);
                                            // notify HWI that snapshot is done
                                            pme->mParent->processSyncEvt(QCAMERA_SM_EVT_SNAPSHOT_DONE,
                                                                         NULL);
                                        }
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
                                cbStatus = BAD_VALUE;
                            }
                            break;
                        };
                    } else {
                        ALOGE("%s : cb message type %d not enabled!",
                              __func__,
                              cb->msg_type);
                        cbStatus = INVALID_OPERATION;
                    }
                    if ( cb->release_cb ) {
                        cb->release_cb(cb->user_data, cb->cookie, cbStatus);
                    }
                    delete cb;
                } else {
                    ALOGE("%s: invalid cb type passed", __func__);
                }
            }
            break;
        case CAMERA_CMD_TYPE_EXIT:
            {
                running = 0;
                pme->mDataQ.flush();
            }
            break;
        default:
            break;
        }
    } while (running);
    CDBG("%s: X", __func__);

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
    if (!mActive) {
        ALOGE("%s: notify thread is not active", __func__);
        return UNKNOWN_ERROR;
    }

    qcamera_callback_argm_t *cbArg = new qcamera_callback_argm_t();
    if (NULL == cbArg) {
        ALOGE("%s: no mem for qcamera_callback_argm_t", __func__);
        return NO_MEMORY;
    }
    memset(cbArg, 0, sizeof(qcamera_callback_argm_t));
    *cbArg = cbArgs;

    if (mDataQ.enqueue((void *)cbArg)) {
        return mProcTh.sendCmd(CAMERA_CMD_TYPE_DO_NEXT_JOB, FALSE, FALSE);
    } else {
        ALOGE("%s: Error adding cb data into queue", __func__);
        delete cbArg;
        return UNKNOWN_ERROR;
    }
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
        mActive = true;
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
