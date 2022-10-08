/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2010, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*!
 * \file      ExynosCameraHWInterface.h
 * \brief     source file for Android Camera HAL
 * \author    thun.hwang(thun.hwang@samsung.com)
 * \date      2010/06/03
 *
 * <b>Revision History: </b>
 * - 2011/12/31 : thun.hwang(thun.hwang@samsung.com) \n
 *   Initial version
 *
 * - 2012/02/01 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Adjust Android Standard features
 *
 * - 2012/03/14 : sangwoo.park(sw5771.park@samsung.com) \n
 *   Change file, class name to ExynosXXX.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraHWInterface"
#include <utils/Log.h>

#include "ExynosCameraHWInterface.h"
#include "exynos_format.h"

#define VIDEO_COMMENT_MARKER_H          (0xFFBE)
#define VIDEO_COMMENT_MARKER_L          (0xFFBF)
#define VIDEO_COMMENT_MARKER_LENGTH     (4)
#define JPEG_EOI_MARKER                 (0xFFD9)
#define HIBYTE(x) (((x) >> 8) & 0xFF)
#define LOBYTE(x) ((x) & 0xFF)

/*TODO: This values will be changed */
#define BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR       "0.10,1.20,Infinity"
#define FRONT_CAMERA_FOCUS_DISTANCES_STR           "0.20,0.25,Infinity"

#define BACK_CAMERA_MACRO_FOCUS_DISTANCES_STR      "0.10,0.20,Infinity"
#define BACK_CAMERA_INFINITY_FOCUS_DISTANCES_STR   "0.10,1.20,Infinity"

#define BACK_CAMERA_FOCUS_DISTANCE_INFINITY        "Infinity"
#define FRONT_CAMERA_FOCUS_DISTANCE_INFINITY       "Infinity"

// This hack does two things:
// -- it sets preview to NV21 (YUV420SP)
// -- it sets gralloc to YV12
//
// The reason being: the samsung encoder understands only yuv420sp, and gralloc
// does yv12 and rgb565.  So what we do is we break up the interleaved UV in
// separate V and U planes, which makes preview look good, and enabled the
// encoder as well.
//
// FIXME: Samsung needs to enable support for proper yv12 coming out of the
//        camera, and to fix their video encoder to work with yv12.
// FIXME: It also seems like either Samsung's YUV420SP (NV21) or img's YV12 has
//        the color planes switched.  We need to figure which side is doing it
//        wrong and have the respective party fix it.

namespace android {

static const int INITIAL_SKIP_FRAME = 8;
static const int EFFECT_SKIP_FRAME = 1;

gralloc_module_t const* ExynosCameraHWInterface::m_grallocHal;

ExynosCameraHWInterface::ExynosCameraHWInterface(int cameraId, camera_device_t *dev)
        :
          m_captureInProgress(false),
          m_skipFrame(0),
          m_notifyCb(0),
          m_dataCb(0),
          m_dataCbTimestamp(0),
          m_callbackCookie(0),
          m_msgEnabled(0),
          m_faceDetected(false),
          m_halDevice(dev),
          m_numOfAvailableVideoBuf(0)
{
    ALOGV("DEBUG(%s):", __func__);
    int ret = 0;

    m_previewWindow = NULL;
    m_secCamera = ExynosCamera::createInstance();

    for (int i = 0; i < NUM_OF_PREVIEW_BUF; i++) {
        m_previewHeap[i] = NULL;
        m_previewBufHandle[i] = NULL;
        m_previewStride[i] = 0;
        m_avaliblePreviewBufHandle[i] = false;
        m_flagGrallocLocked[i] = false;
        m_matchedGrallocIndex[i] = -1;
        m_grallocVirtAddr[i] = NULL;
    }

    m_minUndequeuedBufs = 0;
#ifndef USE_3DNR_DMAOUT
    m_cntVideoBuf = 0;
#endif

    m_oldPictureBufQueueHead = NULL;
    m_getMemoryCb = NULL;
    m_exynosPreviewCSC = NULL;
    m_exynosPictureCSC = NULL;
    m_exynosVideoCSC = NULL;
    m_frameMetadata.number_of_faces = 0;
    m_frameMetadata.faces = m_faces;

    for (int i = 0; i < NUM_OF_VIDEO_BUF; i++) {
        m_videoHeap[i] = NULL;
        m_resizedVideoHeap[i] = NULL;
    }

    m_ion_client = ion_client_create();
    for (int i = 0; i < NUM_OF_PICTURE_BUF; i++)
        m_pictureHeap[i] = NULL;

    m_rawHeap = NULL;

    m_exitAutoFocusThread = false;
    m_exitPreviewThread = false;
    m_exitVideoThread = false;
    /* whether the PreviewThread is active in preview or stopped.  we
     * create the thread but it is initially in stopped state.
     */
    m_previewRunning = false;
    m_videoRunning = false;
    m_pictureRunning = false;
#ifndef USE_3DNR_DMAOUT
    m_videoStart = false;
#endif

    m_previewStartDeferred = false;

    m_recordingHint = false;

    if (!m_grallocHal) {
        ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal);
        if (ret)
            ALOGE("ERR(%s):Fail on loading gralloc HAL", __func__);
    }

    if (m_secCamera->create(cameraId) == false) {
        ALOGE("ERR(%s):Fail on m_secCamera->create(%d)", __func__, cameraId);
        return;
    }

    m_initDefaultParameters(cameraId);

    CSC_METHOD cscMethod = CSC_METHOD_HW;

    m_exynosPreviewCSC = csc_init(cscMethod);
    if (m_exynosPreviewCSC == NULL)
        ALOGE("ERR(%s):csc_init() fail", __func__);

    m_exynosPictureCSC = csc_init(cscMethod);
    if (m_exynosPictureCSC == NULL)
        ALOGE("ERR(%s):csc_init() fail", __func__);

    m_exynosVideoCSC = csc_init(cscMethod);
    if (m_exynosVideoCSC == NULL)
        ALOGE("ERR(%s):csc_init() fail", __func__);

    m_previewThread   = new PreviewThread(this);
    m_videoThread     = new VideoThread(this);
    m_autoFocusThread = new AutoFocusThread(this);
    m_pictureThread   = new PictureThread(this);
}

ExynosCameraHWInterface::~ExynosCameraHWInterface()
{
    close(m_ion_client);
    this->release();
}

status_t ExynosCameraHWInterface::setPreviewWindow(preview_stream_ops *w)
{
    m_previewWindow = w;
    ALOGV("DEBUG(%s):m_previewWindow %p", __func__, m_previewWindow);

    if (m_previewWindow == NULL) {
        ALOGV("DEBUG(%s):preview window is NULL!", __func__);
        return OK;
    }

    m_previewLock.lock();

    if (m_previewRunning == true && m_previewStartDeferred == false) {
        ALOGV("DEBUG(%s):stop preview (window change)", __func__);
        m_stopPreviewInternal();
    }

    if (m_previewWindow->get_min_undequeued_buffer_count(m_previewWindow, &m_minUndequeuedBufs) != 0) {
        ALOGE("ERR(%s):could not retrieve min undequeued buffer count", __func__);
        return INVALID_OPERATION;
    }

    if (NUM_OF_PREVIEW_BUF <= m_minUndequeuedBufs) {
        ALOGE("ERR(%s):min undequeued buffer count %d is too high (expecting at most %d)", __func__,
             m_minUndequeuedBufs, NUM_OF_PREVIEW_BUF - 1);
    }

    if (m_previewWindow->set_buffer_count(m_previewWindow, NUM_OF_PREVIEW_BUF) != 0) {
        ALOGE("ERR(%s):could not set buffer count", __func__);
        return INVALID_OPERATION;
    }

    int previewW, previewH;
    int hal_pixel_format = HAL_PIXEL_FORMAT_YV12;

    m_params.getPreviewSize(&previewW, &previewH);
    const char *str_preview_format = m_params.getPreviewFormat();
    ALOGV("DEBUG(%s):str preview format %s width : %d height : %d ", __func__, str_preview_format, previewW, previewH);

    if (!strcmp(str_preview_format,
                CameraParameters::PIXEL_FORMAT_RGB565)) {
        hal_pixel_format = HAL_PIXEL_FORMAT_RGB_565;
    } else if (!strcmp(str_preview_format,
                     CameraParameters::PIXEL_FORMAT_RGBA8888)) {
        hal_pixel_format = HAL_PIXEL_FORMAT_RGBA_8888;
    } else if (!strcmp(str_preview_format,
                     CameraParameters::PIXEL_FORMAT_YUV420SP)) {
        hal_pixel_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    } else if (!strcmp(str_preview_format,
                     CameraParameters::PIXEL_FORMAT_YUV420P))
        hal_pixel_format = HAL_PIXEL_FORMAT_YV12;

    if (m_previewWindow->set_usage(m_previewWindow,
                                  GRALLOC_USAGE_SW_WRITE_OFTEN |
#ifdef USE_EGL
#else
                                  GRALLOC_USAGE_HWC_HWOVERLAY |
#endif
                                  GRALLOC_USAGE_HW_ION) != 0) {
        ALOGE("ERR(%s):could not set usage on gralloc buffer", __func__);
        return INVALID_OPERATION;
    }

    if (m_previewWindow->set_buffers_geometry(m_previewWindow,
                                              previewW, previewH,
                                              hal_pixel_format) != 0) {
        ALOGE("ERR(%s):could not set buffers geometry to %s",
             __func__, str_preview_format);
        return INVALID_OPERATION;
    }

    if (m_previewRunning == true && m_previewStartDeferred == true) {
        ALOGV("DEBUG(%s):start/resume preview", __func__);
        if (m_startPreviewInternal() == true) {
            m_previewStartDeferred = false;
            m_previewCondition.signal();
        }
    }
    m_previewLock.unlock();

    return OK;
}

void ExynosCameraHWInterface::setCallbacks(camera_notify_callback notify_cb,
                                     camera_data_callback data_cb,
                                     camera_data_timestamp_callback data_cb_timestamp,
                                     camera_request_memory get_memory,
                                     void *user)
{
    m_notifyCb = notify_cb;
    m_dataCb = data_cb;
    m_dataCbTimestamp = data_cb_timestamp;
    m_getMemoryCb = get_memory;
    m_callbackCookie = user;
}

void ExynosCameraHWInterface::enableMsgType(int32_t msgType)
{
    ALOGV("DEBUG(%s):msgType = 0x%x, m_msgEnabled before = 0x%x",
         __func__, msgType, m_msgEnabled);
    m_msgEnabled |= msgType;

    m_previewLock.lock();
    if (   msgType & CAMERA_MSG_PREVIEW_FRAME
        && m_previewRunning == true
        && m_previewStartDeferred == true) {

        ALOGV("DEBUG(%s):starting deferred preview", __func__);

        if (m_startPreviewInternal() == true) {
            m_previewStartDeferred = false;
            m_previewCondition.signal();
        }
    }
    m_previewLock.unlock();

    ALOGV("DEBUG(%s):m_msgEnabled = 0x%x", __func__, m_msgEnabled);
}

void ExynosCameraHWInterface::disableMsgType(int32_t msgType)
{
    ALOGV("DEBUG(%s):msgType = 0x%x, m_msgEnabled before = 0x%x",
         __func__, msgType, m_msgEnabled);
    m_msgEnabled &= ~msgType;
    ALOGV("DEBUG(%s):m_msgEnabled = 0x%x", __func__, m_msgEnabled);
}

bool ExynosCameraHWInterface::msgTypeEnabled(int32_t msgType)
{
    return (m_msgEnabled & msgType);
}

status_t ExynosCameraHWInterface::startPreview()
{
    int ret = OK;

    ALOGV("DEBUG(%s):", __func__);

    Mutex::Autolock lock(m_stateLock);
    if (m_captureInProgress == true) {
        ALOGE("%s : capture in progress, not allowed", __func__);
        return INVALID_OPERATION;
    }

    m_previewLock.lock();
    if (m_previewRunning == true) {
        ALOGE("%s : preview thread already running", __func__);
        m_previewLock.unlock();
        return INVALID_OPERATION;
    }

    m_previewRunning = true;
    m_previewStartDeferred = false;

    if (m_previewWindow == NULL) {
        if (!(m_msgEnabled & CAMERA_MSG_PREVIEW_FRAME)) {
            ALOGV("DEBUG(%s):deferring", __func__);
            m_previewStartDeferred = true;
            m_previewLock.unlock();
            return NO_ERROR;
        }
        ALOGE("%s(%d): m_previewWindow is NULL", __func__, __LINE__);
        return UNKNOWN_ERROR;
    }

    if (m_startPreviewInternal() == true) {
        m_previewCondition.signal();
        ret = OK;
    } else  {
        ret = UNKNOWN_ERROR;
    }

    m_previewLock.unlock();
    return ret;
}

void ExynosCameraHWInterface::stopPreview()
{
    ALOGV("DEBUG(%s):", __func__);

    /* request that the preview thread stop. */
    m_previewLock.lock();
    m_stopPreviewInternal();
    m_previewLock.unlock();
}

bool ExynosCameraHWInterface::previewEnabled()
{
    Mutex::Autolock lock(m_previewLock);
    ALOGV("DEBUG(%s):%d", __func__, m_previewRunning);
    return m_previewRunning;
}

status_t ExynosCameraHWInterface::storeMetaDataInBuffers(bool enable)
{
    if (!enable) {
        ALOGE("Non-m_frameMetadata buffer mode is not supported!");
        return INVALID_OPERATION;
    }
    return OK;
}

status_t ExynosCameraHWInterface::startRecording()
{
    ALOGV("DEBUG(%s):", __func__);

    Mutex::Autolock lock(m_videoLock);

    int videoW, videoH, videoFormat, videoFramesize;

    m_secCamera->getVideoSize(&videoW, &videoH);
    videoFormat = m_secCamera->getVideoFormat();
    videoFramesize = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(videoFormat), videoW, videoH);

    int orgVideoFrameSize = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(videoFormat), m_orgVideoRect.w, m_orgVideoRect.h);

    for (int i = 0; i < NUM_OF_VIDEO_BUF; i++) {

#ifdef USE_3DNR_DMAOUT
        ExynosBuffer videoBuf;

        if (m_videoHeap[i] != NULL) {
            m_videoHeap[i]->release(m_videoHeap[i]);
            m_videoHeap[i] = 0;
        }

        m_videoHeap[i] = m_getMemoryCb(-1, videoFramesize, 1, NULL);
        if (!m_videoHeap[i]) {
            ALOGE("ERR(%s):m_getMemoryCb(m_videoHeap[%d], size(%d) fail", __func__, i, videoFramesize);
            return UNKNOWN_ERROR;
        }

        m_getAlignedYUVSize(videoFormat, videoW, videoH, &videoBuf);

        videoBuf.virt.extP[0] = (char *)m_videoHeap[i]->data;
        for (int j = 1; j < 3; j++) {
            if (videoBuf.size.extS[j] != 0)
                videoBuf.virt.extP[j] = videoBuf.virt.extP[j-1] + videoBuf.size.extS[j-1];
            else
                videoBuf.virt.extP[j] = NULL;
        }

        videoBuf.reserved.p = i;

        m_secCamera->setVideoBuf(&videoBuf);
#endif

        // original VideoSized heap

        if (m_resizedVideoHeap[i] != NULL) {
            m_resizedVideoHeap[i]->release(m_resizedVideoHeap[i]);
            m_resizedVideoHeap[i] = 0;
        }

        m_resizedVideoHeap[i] = m_getMemoryCb(-1, orgVideoFrameSize, 1, NULL);
        if (!m_resizedVideoHeap[i]) {
            ALOGE("ERR(%s):m_getMemoryCb(m_resizedVideoHeap[%d], size(%d) fail", __func__, i, orgVideoFrameSize);
            return UNKNOWN_ERROR;
        }
    }

    if (m_videoRunning == false) {
        if (m_secCamera->startVideo() == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->startVideo()", __func__);
            return UNKNOWN_ERROR;
        }

        m_numOfAvailableVideoBuf = NUM_OF_VIDEO_BUF;

#ifdef USE_3DNR_DMAOUT
        m_videoRunning = true;

        m_videoCondition.signal();
#else
        m_videoStart = true;
#endif
    }

    return NO_ERROR;
}

void ExynosCameraHWInterface::stopRecording()
{
    ALOGV("DEBUG(%s):", __func__);

#ifndef USE_3DNR_DMAOUT
    m_videoStart = false;
#endif

    if (m_videoRunning == true) {
        m_videoRunning = false;

        Mutex::Autolock lock(m_videoLock);

        m_videoCondition.signal();
        /* wait until video thread is stopped */
        m_videoStoppedCondition.wait(m_videoLock);
    } else
        ALOGV("DEBUG(%s):video not running, doing nothing", __func__);
}

bool ExynosCameraHWInterface::recordingEnabled()
{
    return m_videoStart;
}

void ExynosCameraHWInterface::releaseRecordingFrame(const void *opaque)
{
    // This lock makes video lock up
    // Mutex::Autolock lock(m_videoLock);

    int i;
    bool find = false;

    // HACK : this causes recording slow
    /*
    for (i = 0; i < NUM_OF_VIDEO_BUF; i++) {
        if ((char *)m_videoHeap[i]->data == (char *)opaque) {
            find = true;
            break;
        }
    }

    if (find == true) {
        ExynosBuffer videoBuf;
        videoBuf.reserved.p = i;

        m_secCamera->putVideoBuf(&videoBuf);

        m_numOfAvailableVideoBuf++;
        if (NUM_OF_VIDEO_BUF <= m_numOfAvailableVideoBuf)
            m_numOfAvailableVideoBuf = NUM_OF_VIDEO_BUF;
    } else {
        ALOGV("DEBUG(%s):no matched index(%p)", __func__, (char *)opaque);
    }
    */
}

status_t ExynosCameraHWInterface::autoFocus()
{
    ALOGV("DEBUG(%s):", __func__);
    /* signal m_autoFocusThread to run once */
    m_focusCondition.signal();
    return NO_ERROR;
}

status_t ExynosCameraHWInterface::cancelAutoFocus()
{
    if (m_secCamera->cancelAutoFocus() == false) {
        ALOGE("ERR(%s):Fail on m_secCamera->cancelAutoFocus()", __func__);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t ExynosCameraHWInterface::takePicture()
{
    Mutex::Autolock lock(m_stateLock);
    if (m_captureInProgress == true) {
        ALOGE("%s : capture already in progress", __func__);
        return INVALID_OPERATION;
    }

    if (m_pictureRunning == false) {
        ALOGI("%s(%d): m_pictureRunning is false", __func__, __LINE__);
        if (m_startPictureInternal() == false) {
            ALOGE("%s(%d): m_startPictureInternal() fail!!!", __func__, __LINE__);
            return INVALID_OPERATION;
        }
    }

    m_pictureLock.lock();
    m_captureInProgress = true;
    m_pictureLock.unlock();

    if (m_pictureThread->run("CameraPictureThread", PRIORITY_DEFAULT) != NO_ERROR) {
        ALOGE("%s : couldn't run picture thread", __func__);
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosCameraHWInterface::cancelPicture()
{
    ALOGV("DEBUG(%s):", __func__);

    if (m_pictureThread.get()) {
        ALOGV("DEBUG(%s):waiting for picture thread to exit", __func__);
        m_pictureThread->requestExitAndWait();
        ALOGV("DEBUG(%s):picture thread has exited", __func__);
    }

    return NO_ERROR;
}

status_t ExynosCameraHWInterface::setParameters(const CameraParameters& params)
{
    ALOGV("DEBUG(%s):", __func__);

    status_t ret = NO_ERROR;

    /* if someone calls us while picture thread is running, it could screw
     * up the sensor quite a bit so return error.  we can't wait because
     * that would cause deadlock with the callbacks
     */
    m_stateLock.lock();
    if (m_captureInProgress == true) {
        m_stateLock.unlock();
        m_pictureLock.lock();
        m_pictureCondition.waitRelative(m_pictureLock, (2000 * 1000000));
        m_pictureLock.unlock();
    }
    m_stateLock.unlock();

    ///////////////////////////////////////////////////
    // Google Official API : Camera.Parameters
    // http://developer.android.com/reference/android/hardware/Camera.Parameters.html
    ///////////////////////////////////////////////////

    // recording hint
    const char *newRecordingHint = params.get(CameraParameters::KEY_RECORDING_HINT);
    if (newRecordingHint != NULL) {
        if (strcmp(newRecordingHint, "true") == 0)
            m_recordingHint = true;
        else
            m_recordingHint = false;

        m_secCamera->setRecordingHint(m_recordingHint);
    }

    // preview size
    int newPreviewW = 0;
    int newPreviewH = 0;
    int newCalPreviewW = 0;
    int newCalPreviewH = 0;
    int previewMaxW  = 0;
    int previewMaxH  = 0;
    params.getPreviewSize(&newPreviewW, &newPreviewH);

    // In general, it will show preview max size
    m_secCamera->getSupportedPreviewSizes(&previewMaxW, &previewMaxH);
    newCalPreviewW = previewMaxW;
    newCalPreviewH = previewMaxH;

    // When recording, it will show video max size
    if (m_recordingHint == true) {
        m_secCamera->getSupportedVideoSizes(&newCalPreviewW, &newCalPreviewH);
        if (   previewMaxW < newCalPreviewW
            || previewMaxH < newCalPreviewH) {
            newCalPreviewW = previewMaxW;
            newCalPreviewH = previewMaxH;
        }
    }

    m_orgPreviewRect.w = newPreviewW;
    m_orgPreviewRect.h = newPreviewH;

    // TODO : calibrate original preview ratio
    //m_getRatioSize(newCalPreviewW, newCalPreviewH, newPreviewW, newPreviewH, &newPreviewW, &newPreviewH);
    newPreviewW = newCalPreviewW;
    newPreviewH = newCalPreviewH;

    const char *strNewPreviewFormat = params.getPreviewFormat();
    ALOGV("DEBUG(%s):newPreviewW x newPreviewH = %dx%d, format = %s",
         __func__, newPreviewW, newPreviewH, strNewPreviewFormat);

    if (0 < newPreviewW &&
        0 < newPreviewH &&
        strNewPreviewFormat != NULL &&
        m_isSupportedPreviewSize(newPreviewW, newPreviewH) == true) {
        int newPreviewFormat = 0;

        if (!strcmp(strNewPreviewFormat, CameraParameters::PIXEL_FORMAT_RGB565))
            newPreviewFormat = V4L2_PIX_FMT_RGB565;
        else if (!strcmp(strNewPreviewFormat, CameraParameters::PIXEL_FORMAT_RGBA8888))
            newPreviewFormat = V4L2_PIX_FMT_RGB32;
        else if (!strcmp(strNewPreviewFormat, CameraParameters::PIXEL_FORMAT_YUV420SP))
            newPreviewFormat = V4L2_PIX_FMT_NV21;
        else if (!strcmp(strNewPreviewFormat, CameraParameters::PIXEL_FORMAT_YUV420P))
            newPreviewFormat = V4L2_PIX_FMT_YVU420M;
        else if (!strcmp(strNewPreviewFormat, "yuv420sp_custom"))
            newPreviewFormat = V4L2_PIX_FMT_NV12T;
        else if (!strcmp(strNewPreviewFormat, "yuv422i"))
            newPreviewFormat = V4L2_PIX_FMT_YUYV;
        else if (!strcmp(strNewPreviewFormat, "yuv422p"))
            newPreviewFormat = V4L2_PIX_FMT_YUV422P;
        else
            newPreviewFormat = V4L2_PIX_FMT_NV21; //for 3rd party

        m_orgPreviewRect.colorFormat = newPreviewFormat;

        int curPreviewW, curPreviewH;
        m_secCamera->getPreviewSize(&curPreviewW, &curPreviewH);
        int curPreviewFormat = m_secCamera->getPreviewFormat();

        if (curPreviewW != newPreviewW ||
            curPreviewH != newPreviewH ||
            curPreviewFormat != newPreviewFormat) {
            if (   m_secCamera->setPreviewSize(newPreviewW, newPreviewH) == false
                || m_secCamera->setPreviewFormat(newPreviewFormat) == false) {
                ALOGE("ERR(%s):Fail on m_secCamera->setPreviewSize(width(%d), height(%d), format(%d))",
                     __func__, newPreviewW, newPreviewH, newPreviewFormat);
                ret = UNKNOWN_ERROR;
            } else {
                if (m_previewWindow) {
                    if (m_previewRunning == true && m_previewStartDeferred == false) {
                        ALOGE("ERR(%s):preview is running, cannot change size and format!", __func__);
                        ret = INVALID_OPERATION;
                    }

                    ALOGV("DEBUG(%s):m_previewWindow (%p) set_buffers_geometry", __func__, m_previewWindow);
                    ALOGV("DEBUG(%s):m_previewWindow->set_buffers_geometry (%p)", __func__,
                         m_previewWindow->set_buffers_geometry);
                    m_previewWindow->set_buffers_geometry(m_previewWindow,
                                                         newPreviewW, newPreviewH,
                                                         newPreviewFormat);
                    ALOGV("DEBUG(%s):DONE m_previewWindow (%p) set_buffers_geometry", __func__, m_previewWindow);
                }
                m_params.setPreviewSize(newPreviewW, newPreviewH);
                m_params.setPreviewFormat(strNewPreviewFormat);
            }
        }
        else {
            ALOGV("DEBUG(%s):preview size and format has not changed", __func__);
        }
    } else {
        ALOGE("ERR(%s):Invalid preview size(%dx%d)", __func__, newPreviewW, newPreviewH);
        ret = INVALID_OPERATION;
    }

    int newPictureW = 0;
    int newPictureH = 0;
    params.getPictureSize(&newPictureW, &newPictureH);
    ALOGV("DEBUG(%s):newPictureW x newPictureH = %dx%d", __func__, newPictureW, newPictureH);

    if (0 < newPictureW && 0 < newPictureH) {

        int orgPictureW, orgPictureH = 0;
        m_secCamera->getPictureSize(&orgPictureW, &orgPictureH);

        if (m_secCamera->setPictureSize(newPictureW, newPictureH) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setPictureSize(width(%d), height(%d))",
                    __func__, newPictureW, newPictureH);
            ret = UNKNOWN_ERROR;
        } else {
            int tempW, tempH = 0;
            m_secCamera->getPictureSize(&tempW, &tempH);

            if (tempW != orgPictureW || tempH != orgPictureH) {

                if (m_pictureRunning == true) {
                    if (m_stopPictureInternal() == false)
                        ALOGE("ERR(%s):m_stopPictureInternal() fail", __func__);

                    if (m_startPictureInternal() == false)
                        ALOGE("ERR(%s):m_startPictureInternal() fail", __func__);
                }
            }
            m_orgPictureRect.w = newPictureW;
            m_orgPictureRect.h = newPictureH;
            m_params.setPictureSize(newPictureW, newPictureH);
        }
    }

    // picture format
    const char *newPictureFormat = params.getPictureFormat();
    ALOGV("DEBUG(%s):newPictureFormat %s", __func__, newPictureFormat);

    if (newPictureFormat != NULL) {
        int value = 0;

        if (!strcmp(newPictureFormat, CameraParameters::PIXEL_FORMAT_RGB565))
            value = V4L2_PIX_FMT_RGB565;
        else if (!strcmp(newPictureFormat, CameraParameters::PIXEL_FORMAT_RGBA8888))
            value = V4L2_PIX_FMT_RGB32;
        else if (!strcmp(newPictureFormat, CameraParameters::PIXEL_FORMAT_YUV420SP))
            value = V4L2_PIX_FMT_NV21;
        else if (!strcmp(newPictureFormat, "yuv420sp_custom"))
            value = V4L2_PIX_FMT_NV12T;
        else if (!strcmp(newPictureFormat, "yuv420p"))
            value = V4L2_PIX_FMT_YUV420;
        else if (!strcmp(newPictureFormat, "yuv422i"))
            value = V4L2_PIX_FMT_YUYV;
        else if (!strcmp(newPictureFormat, "uyv422i_custom")) //Zero copy UYVY format
            value = V4L2_PIX_FMT_UYVY;
        else if (!strcmp(newPictureFormat, "uyv422i")) //Non-zero copy UYVY format
            value = V4L2_PIX_FMT_UYVY;
        else if (!strcmp(newPictureFormat, CameraParameters::PIXEL_FORMAT_JPEG))
            value = V4L2_PIX_FMT_YUYV;
        else if (!strcmp(newPictureFormat, "yuv422p"))
            value = V4L2_PIX_FMT_YUV422P;
        else
            value = V4L2_PIX_FMT_NV21; //for 3rd party

        if (value != m_secCamera->getPictureFormat()) {
            if (m_secCamera->setPictureFormat(value) == false) {
                ALOGE("ERR(%s):Fail on m_secCamera->setPictureFormat(format(%d))", __func__, value);
                ret = UNKNOWN_ERROR;
            } else {
                m_orgPictureRect.colorFormat = value;
                m_params.setPictureFormat(newPictureFormat);
            }
        }
    }

    // JPEG image quality
    int newJpegQuality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    ALOGV("DEBUG(%s):newJpegQuality %d", __func__, newJpegQuality);
    // we ignore bad values
    if (newJpegQuality >=1 && newJpegQuality <= 100) {
        if (m_secCamera->setJpegQuality(newJpegQuality) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setJpegQuality(quality(%d))", __func__, newJpegQuality);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set(CameraParameters::KEY_JPEG_QUALITY, newJpegQuality);
        }
    }

    // JPEG thumbnail size
    int newJpegThumbnailW = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    int newJpegThumbnailH = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    if (0 <= newJpegThumbnailW && 0 <= newJpegThumbnailH) {
        if (m_secCamera->setJpegThumbnailSize(newJpegThumbnailW, newJpegThumbnailH) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setJpegThumbnailSize(width(%d), height(%d))", __func__, newJpegThumbnailW, newJpegThumbnailH);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,  newJpegThumbnailW);
            m_params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, newJpegThumbnailH);
        }
    }

    // JPEG thumbnail quality
    int newJpegThumbnailQuality = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    ALOGV("DEBUG(%s):newJpegThumbnailQuality %d", __func__, newJpegThumbnailQuality);
    // we ignore bad values
    if (newJpegThumbnailQuality >=1 && newJpegThumbnailQuality <= 100) {
        if (m_secCamera->setJpegThumbnailQuality(newJpegThumbnailQuality) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setJpegThumbnailQuality(quality(%d))",
                                               __func__, newJpegThumbnailQuality);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, newJpegThumbnailQuality);
        }
    }

    // Video size
    int newVideoW = 0;
    int newVideoH = 0;
    params.getVideoSize(&newVideoW, &newVideoH);
    ALOGV("DEBUG(%s):newVideoW (%d) newVideoH (%d)", __func__, newVideoW, newVideoH);
    if (0 < newVideoW && 0 < newVideoH && m_videoStart == false) {

        m_orgVideoRect.w = newVideoW;
        m_orgVideoRect.h = newVideoH;

        if (m_secCamera->setVideoSize(newVideoW, newVideoH) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setVideoSize(width(%d), height(%d))",
            __func__, newVideoW, newVideoH);
            ret = UNKNOWN_ERROR;
        }
        m_params.setVideoSize(newVideoW, newVideoH);
    }

    // video stablization
    const char *newVideoStabilization = params.get(CameraParameters::KEY_VIDEO_STABILIZATION);
    bool currVideoStabilization = m_secCamera->getVideoStabilization();
    ALOGV("DEBUG(%s):newVideoStabilization %s", __func__, newVideoStabilization);
    if (newVideoStabilization != NULL) {
        bool toggle = false;

        if (!strcmp(newVideoStabilization, "true"))
            toggle = true;

        if ( currVideoStabilization != toggle) {
            if (m_secCamera->setVideoStabilization(toggle) == false) {
                ALOGE("ERR(%s):setVideoStabilization() fail", __func__);
                ret = UNKNOWN_ERROR;
            } else {
                m_params.set(CameraParameters::KEY_VIDEO_STABILIZATION, newVideoStabilization);
            }
        }
    }

    // 3dnr
    const char *new3dnr = params.get("3dnr");
    ALOGV("DEBUG(%s):new3drn %s", __func__, new3dnr);
    if (new3dnr != NULL) {
        bool toggle = false;

        if (!strcmp(new3dnr, "true"))
            toggle = true;

            if (m_secCamera->set3DNR(toggle) == false) {
                ALOGE("ERR(%s):set3DNR() fail", __func__);
                ret = UNKNOWN_ERROR;
            } else {
                m_params.set("3dnr", new3dnr);
            }
    }

    // odc
    const char *newOdc = params.get("odc");
    ALOGV("DEBUG(%s):newOdc %s", __func__, new3dnr);
    if (newOdc != NULL) {
        bool toggle = false;

        if (!strcmp(newOdc, "true"))
            toggle = true;

            if (m_secCamera->setODC(toggle) == false) {
                ALOGE("ERR(%s):setODC() fail", __func__);
                ret = UNKNOWN_ERROR;
            } else {
                m_params.set("odc", newOdc);
            }
    }

    // frame rate
    int newFrameRate = params.getPreviewFrameRate();
    ALOGV("DEBUG(%s):newFrameRate %d", __func__, newFrameRate);
    // ignore any fps request, we're determine fps automatically based
    // on scene mode.  don't return an error because it causes CTS failure.
    if (newFrameRate != m_params.getPreviewFrameRate()) {
        if (m_secCamera->setPreviewFrameRate(newFrameRate) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setPreviewFrameRate(%d)", __func__, newFrameRate);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.setPreviewFrameRate(newFrameRate);
        }
    }

    // zoom
    int newZoom = params.getInt(CameraParameters::KEY_ZOOM);
    ALOGV("DEBUG(%s):newZoom %d", __func__, newZoom);
    if (0 <= newZoom) {
        if (m_secCamera->setZoom(newZoom) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setZoom(newZoom(%d))", __func__, newZoom);
            ret = UNKNOWN_ERROR;
        }
        else {
            m_params.set(CameraParameters::KEY_ZOOM, newZoom);
        }
    }

    // rotation
    int newRotation = params.getInt(CameraParameters::KEY_ROTATION);
    ALOGV("DEBUG(%s):newRotation %d", __func__, newRotation);
    if (0 <= newRotation) {
        ALOGV("DEBUG(%s):set orientation:%d", __func__, newRotation);
        if (m_secCamera->setRotation(newRotation) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setRotation(%d)", __func__, newRotation);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set(CameraParameters::KEY_ROTATION, newRotation);
        }
    }

    // auto exposure lock
    const char *newAutoExposureLock = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
    if (newAutoExposureLock != NULL) {
        bool toggle = false;

        if (!strcmp(newAutoExposureLock, "true"))
            toggle = true;

        if (m_secCamera->setAutoExposureLock(toggle) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setAutoExposureLock()", __func__);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, newAutoExposureLock);
        }
    }

    // exposure
    int minExposureCompensation = params.getInt(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
    int maxExposureCompensation = params.getInt(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION);
    int newExposureCompensation = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    ALOGV("DEBUG(%s):newExposureCompensation %d", __func__, newExposureCompensation);
    if ((minExposureCompensation <= newExposureCompensation) &&
        (newExposureCompensation <= maxExposureCompensation)) {
        if (m_secCamera->setExposureCompensation(newExposureCompensation) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setExposureCompensation(exposure(%d))", __func__, newExposureCompensation);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, newExposureCompensation);
        }
    }

    // auto white balance lock
    const char *newAutoWhitebalanceLock = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
    if (newAutoWhitebalanceLock != NULL) {
        bool toggle = false;

        if (!strcmp(newAutoWhitebalanceLock, "true"))
            toggle = true;

        if (m_secCamera->setAutoWhiteBalanceLock(toggle) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setAutoWhiteBalanceLock()", __func__);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, newAutoWhitebalanceLock);
        }
    }

    // white balance
    const char *newWhiteBalance = params.get(CameraParameters::KEY_WHITE_BALANCE);
    ALOGV("DEBUG(%s):newWhiteBalance %s", __func__, newWhiteBalance);
    if (newWhiteBalance != NULL) {
        int value = -1;

        if (!strcmp(newWhiteBalance, CameraParameters::WHITE_BALANCE_AUTO))
            value = ExynosCamera::WHITE_BALANCE_AUTO;
        else if (!strcmp(newWhiteBalance, CameraParameters::WHITE_BALANCE_INCANDESCENT))
            value = ExynosCamera::WHITE_BALANCE_INCANDESCENT;
        else if (!strcmp(newWhiteBalance, CameraParameters::WHITE_BALANCE_FLUORESCENT))
            value = ExynosCamera::WHITE_BALANCE_FLUORESCENT;
        else if (!strcmp(newWhiteBalance, CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT))
            value = ExynosCamera::WHITE_BALANCE_WARM_FLUORESCENT;
        else if (!strcmp(newWhiteBalance, CameraParameters::WHITE_BALANCE_DAYLIGHT))
            value = ExynosCamera::WHITE_BALANCE_DAYLIGHT;
        else if (!strcmp(newWhiteBalance, CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT))
            value = ExynosCamera::WHITE_BALANCE_CLOUDY_DAYLIGHT;
        else if (!strcmp(newWhiteBalance, CameraParameters::WHITE_BALANCE_TWILIGHT))
            value = ExynosCamera::WHITE_BALANCE_TWILIGHT;
        else if (!strcmp(newWhiteBalance, CameraParameters::WHITE_BALANCE_SHADE))
            value = ExynosCamera::WHITE_BALANCE_SHADE;
        else {
            ALOGE("ERR(%s):Invalid white balance(%s)", __func__, newWhiteBalance); //twilight, shade, warm_flourescent
            ret = UNKNOWN_ERROR;
        }

        if (0 <= value) {
            if (m_secCamera->setWhiteBalance(value) == false) {
                ALOGE("ERR(%s):Fail on m_secCamera->setWhiteBalance(white(%d))", __func__, value);
                ret = UNKNOWN_ERROR;
            } else {
                m_params.set(CameraParameters::KEY_WHITE_BALANCE, newWhiteBalance);
            }
        }
    }

    // Metering
    // This is the additional API(not Google API).
    // But, This is set berfore the below KEY_METERING_AREAS.
    const char *strNewMetering = params.get("metering");
    ALOGV("DEBUG(%s):strNewMetering %s", __func__, strNewMetering);
    if (strNewMetering != NULL) {
        int newMetering = -1;

        if (!strcmp(strNewMetering, "average"))
            newMetering = ExynosCamera::METERING_MODE_AVERAGE;
        else if (!strcmp(strNewMetering, "center"))
            newMetering = ExynosCamera::METERING_MODE_CENTER;
        else if (!strcmp(strNewMetering, "matrix"))
            newMetering = ExynosCamera::METERING_MODE_MATRIX;
        else if (!strcmp(strNewMetering, "spot"))
            newMetering = ExynosCamera::METERING_MODE_SPOT;
        else {
            ALOGE("ERR(%s):Invalid metering newMetering(%s)", __func__, strNewMetering);
            ret = UNKNOWN_ERROR;
        }

        if (0 <= newMetering) {
            if (m_secCamera->setMeteringMode(newMetering) == false) {
                ALOGE("ERR(%s):Fail on m_secCamera->setMeteringMode(%d)", __func__, newMetering);
                ret = UNKNOWN_ERROR;
            } else {
                m_params.set("metering", strNewMetering);
            }
        }
    }

    // metering areas
    const char *newMeteringAreas = params.get(CameraParameters::KEY_METERING_AREAS);
    int maxNumMeteringAreas = m_secCamera->getMaxNumMeteringAreas();

    if (newMeteringAreas != NULL && maxNumMeteringAreas != 0) {
        // ex : (-10,-10,0,0,300),(0,0,10,10,700)
        ExynosRect2 *rect2s  = new ExynosRect2[maxNumMeteringAreas];
        int         *weights = new int[maxNumMeteringAreas];

        int validMeteringAreas = m_bracketsStr2Ints((char *)newMeteringAreas, maxNumMeteringAreas, rect2s, weights);
        if (0 < validMeteringAreas) {
            for (int i = 0; i < validMeteringAreas; i++) {
                rect2s[i].x1 = m_calibratePosition(2000, newPreviewW, rect2s[i].x1 + 1000);
                rect2s[i].y1 = m_calibratePosition(2000, newPreviewH, rect2s[i].y1 + 1000);
                rect2s[i].x2 = m_calibratePosition(2000, newPreviewW, rect2s[i].x2 + 1000);
                rect2s[i].y2 = m_calibratePosition(2000, newPreviewH, rect2s[i].y2 + 1000);
            }

            if (m_secCamera->setMeteringAreas(validMeteringAreas, rect2s, weights) == false) {
                ALOGE("ERR(%s):setMeteringAreas(%s) fail", __func__, newMeteringAreas);
                ret = UNKNOWN_ERROR;
            }
            else {
                m_params.set(CameraParameters::KEY_METERING_AREAS, newMeteringAreas);
            }
        }

        delete [] rect2s;
        delete [] weights;
    }

    // anti banding
    const char *newAntibanding = params.get(CameraParameters::KEY_ANTIBANDING);
    ALOGV("DEBUG(%s):newAntibanding %s", __func__, newAntibanding);
    if (newAntibanding != NULL) {
        int value = -1;

        if (!strcmp(newAntibanding, CameraParameters::ANTIBANDING_AUTO))
            value = ExynosCamera::ANTIBANDING_AUTO;
        else if (!strcmp(newAntibanding, CameraParameters::ANTIBANDING_50HZ))
            value = ExynosCamera::ANTIBANDING_50HZ;
        else if (!strcmp(newAntibanding, CameraParameters::ANTIBANDING_60HZ))
            value = ExynosCamera::ANTIBANDING_60HZ;
        else if (!strcmp(newAntibanding, CameraParameters::ANTIBANDING_OFF))
            value = ExynosCamera::ANTIBANDING_OFF;
        else {
            ALOGE("ERR(%s):Invalid antibanding value(%s)", __func__, newAntibanding);
            ret = UNKNOWN_ERROR;
        }

        if (0 <= value) {
            if (m_secCamera->setAntibanding(value) == false) {
                ALOGE("ERR(%s):Fail on m_secCamera->setAntibanding(%d)", __func__, value);
                ret = UNKNOWN_ERROR;
            } else {
                m_params.set(CameraParameters::KEY_ANTIBANDING, newAntibanding);
            }
        }
    }

    // scene mode
    const char *strNewSceneMode = params.get(CameraParameters::KEY_SCENE_MODE);
    const char *strCurSceneMode = m_params.get(CameraParameters::KEY_SCENE_MODE);

    // fps range
    int newMinFps = 0;
    int newMaxFps = 0;
    int curMinFps = 0;
    int curMaxFps = 0;
    params.getPreviewFpsRange(&newMinFps, &newMaxFps);
    m_params.getPreviewFpsRange(&curMinFps, &curMaxFps);
    /* our fps range is determined by the sensor, reject any request
     * that isn't exactly what we're already at.
     * but the check is performed when requesting only changing fps range
     */
    if (strNewSceneMode && strCurSceneMode) {
        if (!strcmp(strNewSceneMode, strCurSceneMode)) {
            if ((newMinFps != curMinFps) || (newMaxFps != curMaxFps)) {
                ALOGW("%s : requested newMinFps = %d, newMaxFps = %d not allowed",
                        __func__, newMinFps, newMaxFps);
                ALOGE("%s : curMinFps = %d, curMaxFps = %d",
                        __func__, curMinFps, curMaxFps);
                ret = UNKNOWN_ERROR;
            }
        }
    } else {
        /* Check basic validation if scene mode is different */
        if ((newMaxFps < newMinFps) ||
            (newMinFps < 0) || (newMaxFps < 0))
        ret = UNKNOWN_ERROR;
    }

    if (strNewSceneMode != NULL) {
        int  newSceneMode = -1;

        const char *strNewFlashMode = params.get(CameraParameters::KEY_FLASH_MODE);
        const char *strNewFocusMode = params.get(CameraParameters::KEY_FOCUS_MODE);

        // fps range is (15000,30000) by default.
        m_params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(15000,30000)");
        m_params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "15000,30000");

        if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_AUTO)) {
            newSceneMode = ExynosCamera::SCENE_MODE_AUTO;
        } else {
            // defaults for non-auto scene modes
            if (m_secCamera->getSupportedFocusModes() != 0)
                strNewFocusMode = CameraParameters::FOCUS_MODE_AUTO;

            strNewFlashMode = CameraParameters::FLASH_MODE_OFF;

            if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_ACTION)) {
                newSceneMode = ExynosCamera::SCENE_MODE_ACTION;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_PORTRAIT)) {
                newSceneMode = ExynosCamera::SCENE_MODE_PORTRAIT;
                strNewFlashMode = CameraParameters::FLASH_MODE_AUTO;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_LANDSCAPE)) {
                newSceneMode = ExynosCamera::SCENE_MODE_LANDSCAPE;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_NIGHT)) {
                newSceneMode = ExynosCamera::SCENE_MODE_NIGHT;
                m_params.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(4000,30000)");
                m_params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "4000,30000");
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_NIGHT_PORTRAIT)) {
                newSceneMode = ExynosCamera::SCENE_MODE_NIGHT_PORTRAIT;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_THEATRE)) {
                newSceneMode = ExynosCamera::SCENE_MODE_THEATRE;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_BEACH)) {
                newSceneMode = ExynosCamera::SCENE_MODE_BEACH;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_SNOW)) {
                newSceneMode = ExynosCamera::SCENE_MODE_SNOW;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_SUNSET)) {
                newSceneMode = ExynosCamera::SCENE_MODE_SUNSET;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_STEADYPHOTO)) {
                newSceneMode = ExynosCamera::SCENE_MODE_STEADYPHOTO;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_FIREWORKS)) {
                newSceneMode = ExynosCamera::SCENE_MODE_FIREWORKS;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_SPORTS)) {
                newSceneMode = ExynosCamera::SCENE_MODE_SPORTS;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_PARTY)) {
                newSceneMode = ExynosCamera::SCENE_MODE_PARTY;
                strNewFlashMode = CameraParameters::FLASH_MODE_AUTO;
            } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_CANDLELIGHT)) {
                newSceneMode = ExynosCamera::SCENE_MODE_CANDLELIGHT;
            } else {
                ALOGE("ERR(%s):unmatched scene_mode(%s)",
                        __func__, strNewSceneMode); //action, night-portrait, theatre, steadyphoto
                ret = UNKNOWN_ERROR;
            }
        }

        // focus mode
        if (strNewFocusMode != NULL) {
            int  newFocusMode = -1;

            if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_AUTO)) {
                newFocusMode = ExynosCamera::FOCUS_MODE_AUTO;
                m_params.set(CameraParameters::KEY_FOCUS_DISTANCES,
                                BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR);
            } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_INFINITY)) {
                newFocusMode = ExynosCamera::FOCUS_MODE_INFINITY;
                m_params.set(CameraParameters::KEY_FOCUS_DISTANCES,
                                BACK_CAMERA_INFINITY_FOCUS_DISTANCES_STR);
            } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_MACRO)) {
                newFocusMode = ExynosCamera::FOCUS_MODE_MACRO;
                m_params.set(CameraParameters::KEY_FOCUS_DISTANCES,
                                BACK_CAMERA_MACRO_FOCUS_DISTANCES_STR);
            } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_FIXED)) {
                newFocusMode = ExynosCamera::FOCUS_MODE_FIXED;
            } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_EDOF)) {
                newFocusMode = ExynosCamera::FOCUS_MODE_EDOF;
            } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO)) {
                newFocusMode = ExynosCamera::FOCUS_MODE_CONTINUOUS_VIDEO;
            } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE)) {
                newFocusMode = ExynosCamera::FOCUS_MODE_CONTINUOUS_PICTURE;
            } else {
                ALOGE("ERR(%s):unmatched focus_mode(%s)", __func__, strNewFocusMode);
                ret = UNKNOWN_ERROR;
            }

            if (0 <= newFocusMode) {
                if (m_secCamera->setFocusMode(newFocusMode) == false) {
                    ALOGE("ERR(%s):m_secCamera->setFocusMode(%d) fail", __func__, newFocusMode);
                    ret = UNKNOWN_ERROR;
                } else {
                    m_params.set(CameraParameters::KEY_FOCUS_MODE, strNewFocusMode);
                }
            }
        }

        // flash mode
        if (strNewFlashMode != NULL) {
            int  newFlashMode = -1;

            if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_OFF))
                newFlashMode = ExynosCamera::FLASH_MODE_OFF;
            else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_AUTO))
                newFlashMode = ExynosCamera::FLASH_MODE_AUTO;
            else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_ON))
                newFlashMode = ExynosCamera::FLASH_MODE_ON;
            else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_RED_EYE))
                newFlashMode = ExynosCamera::FLASH_MODE_RED_EYE;
            else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_TORCH))
                newFlashMode = ExynosCamera::FLASH_MODE_TORCH;
            else {
                ALOGE("ERR(%s):unmatched flash_mode(%s)", __func__, strNewFlashMode); //red-eye
                ret = UNKNOWN_ERROR;
            }
            if (0 <= newFlashMode) {
                if (m_secCamera->setFlashMode(newFlashMode) == false) {
                    ALOGE("ERR(%s):m_secCamera->setFlashMode(%d) fail", __func__, newFlashMode);
                    ret = UNKNOWN_ERROR;
                } else {
                    m_params.set(CameraParameters::KEY_FLASH_MODE, strNewFlashMode);
                }
            }
        }

        // scene mode
        if (0 <= newSceneMode) {
            if (m_secCamera->setSceneMode(newSceneMode) == false) {
                ALOGE("ERR(%s):m_secCamera->setSceneMode(%d) fail", __func__, newSceneMode);
                ret = UNKNOWN_ERROR;
            } else {
                m_params.set(CameraParameters::KEY_SCENE_MODE, strNewSceneMode);
            }
        }
    }

    // focus areas
    const char *newFocusAreas = params.get(CameraParameters::KEY_FOCUS_AREAS);
    int maxNumFocusAreas = m_secCamera->getMaxNumFocusAreas();

    if (newFocusAreas != NULL && maxNumFocusAreas != 0) {
        int curFocusMode = m_secCamera->getFocusMode();

        // In CameraParameters.h
        // Focus area only has effect if the cur focus mode is FOCUS_MODE_AUTO,
        // FOCUS_MODE_MACRO, FOCUS_MODE_CONTINUOUS_VIDEO, or
        // FOCUS_MODE_CONTINUOUS_PICTURE.
        if (   curFocusMode & ExynosCamera::FOCUS_MODE_AUTO
            || curFocusMode & ExynosCamera::FOCUS_MODE_MACRO
            || curFocusMode & ExynosCamera::FOCUS_MODE_CONTINUOUS_VIDEO
            || curFocusMode & ExynosCamera::FOCUS_MODE_CONTINUOUS_PICTURE) {

            // ex : (-10,-10,0,0,300),(0,0,10,10,700)
            ExynosRect2 *rect2s = new ExynosRect2[maxNumFocusAreas];
            int         *weights = new int[maxNumFocusAreas];

            int validFocusedAreas = m_bracketsStr2Ints((char *)newFocusAreas, maxNumFocusAreas, rect2s, weights);
            if (0 < validFocusedAreas) {
                // CameraParameters.h
                // A special case of single focus area (0,0,0,0,0) means driver to decide
                // the focus area. For example, the driver may use more signals to decide
                // focus areas and change them dynamically. Apps can set (0,0,0,0,0) if they
                // want the driver to decide focus areas.
                if (   validFocusedAreas == 1
                    && rect2s[0].x1 == 0 && rect2s[0].y1 == 0 && rect2s[0].x2 == 0 && rect2s[0].y2 == 0) {
                    rect2s[0].x1 = 0;
                    rect2s[0].y1 = 0;
                    rect2s[0].x2 = newPreviewW;
                    rect2s[0].y2 = newPreviewH;
                } else {
                    for (int i = 0; i < validFocusedAreas; i++) {
                        rect2s[i].x1 = (rect2s[i].x1 + 1000) * 1023 / 2000;
                        rect2s[i].y1 = (rect2s[i].y1 + 1000) * 1023 / 2000;
                        rect2s[i].x2 = (rect2s[i].x2 + 1000) * 1023 / 2000;
                        rect2s[i].y2 = (rect2s[i].y2 + 1000) * 1023 / 2000;
                    }

                    if (m_secCamera->setFocusAreas(validFocusedAreas, rect2s, weights) == false) {
                        ALOGE("ERR(%s):setFocusAreas(%s) fail", __func__, newFocusAreas);
                        ret = UNKNOWN_ERROR;
                    } else {
                        m_params.set(CameraParameters::KEY_FOCUS_AREAS, newFocusAreas);
                    }
                }
            }

            delete [] rect2s;
            delete [] weights;
        }
    }

    // image effect
    const char *strNewEffect = params.get(CameraParameters::KEY_EFFECT);
    if (strNewEffect != NULL) {

        int  newEffect = -1;

        if (!strcmp(strNewEffect, CameraParameters::EFFECT_NONE)) {
            newEffect = ExynosCamera::EFFECT_NONE;
        } else if (!strcmp(strNewEffect, CameraParameters::EFFECT_MONO)) {
            newEffect = ExynosCamera::EFFECT_MONO;
        } else if (!strcmp(strNewEffect, CameraParameters::EFFECT_NEGATIVE)) {
            newEffect = ExynosCamera::EFFECT_NEGATIVE;
        } else if (!strcmp(strNewEffect, CameraParameters::EFFECT_SOLARIZE)) {
            newEffect = ExynosCamera::EFFECT_SOLARIZE;
        } else if (!strcmp(strNewEffect, CameraParameters::EFFECT_SEPIA)) {
            newEffect = ExynosCamera::EFFECT_SEPIA;
        } else if (!strcmp(strNewEffect, CameraParameters::EFFECT_POSTERIZE)) {
            newEffect = ExynosCamera::EFFECT_POSTERIZE;
        } else if (!strcmp(strNewEffect, CameraParameters::EFFECT_WHITEBOARD)) {
            newEffect = ExynosCamera::EFFECT_WHITEBOARD;
        } else if (!strcmp(strNewEffect, CameraParameters::EFFECT_BLACKBOARD)) {
            newEffect = ExynosCamera::EFFECT_BLACKBOARD;
        } else if (!strcmp(strNewEffect, CameraParameters::EFFECT_AQUA)) {
            newEffect = ExynosCamera::EFFECT_AQUA;
        } else {
            ALOGE("ERR(%s):Invalid effect(%s)", __func__, strNewEffect);
            ret = UNKNOWN_ERROR;
        }

        if (0 <= newEffect) {
            if (m_secCamera->setColorEffect(newEffect) == false) {
                ALOGE("ERR(%s):Fail on m_secCamera->setColorEffect(effect(%d))", __func__, newEffect);
                ret = UNKNOWN_ERROR;
            } else {
                const char *oldStrEffect = m_params.get(CameraParameters::KEY_EFFECT);

                if (oldStrEffect) {
                    if (strcmp(oldStrEffect, strNewEffect)) {
                        m_setSkipFrame(EFFECT_SKIP_FRAME);
                    }
                }
                m_params.set(CameraParameters::KEY_EFFECT, strNewEffect);
            }
        }
    }

    // gps altitude
    const char *strNewGpsAltitude = params.get(CameraParameters::KEY_GPS_ALTITUDE);

    if (m_secCamera->setGpsAltitude(strNewGpsAltitude) == false) {
        ALOGE("ERR(%s):m_secCamera->setGpsAltitude(%s) fail", __func__, strNewGpsAltitude);
        ret = UNKNOWN_ERROR;
    } else {
        if (strNewGpsAltitude)
            m_params.set(CameraParameters::KEY_GPS_ALTITUDE, strNewGpsAltitude);
        else
            m_params.remove(CameraParameters::KEY_GPS_ALTITUDE);
    }

    // gps latitude
    const char *strNewGpsLatitude = params.get(CameraParameters::KEY_GPS_LATITUDE);
    if (m_secCamera->setGpsLatitude(strNewGpsLatitude) == false) {
        ALOGE("ERR(%s):m_secCamera->setGpsLatitude(%s) fail", __func__, strNewGpsLatitude);
        ret = UNKNOWN_ERROR;
    } else {
        if (strNewGpsLatitude)
            m_params.set(CameraParameters::KEY_GPS_LATITUDE, strNewGpsLatitude);
        else
            m_params.remove(CameraParameters::KEY_GPS_LATITUDE);
    }

    // gps longitude
    const char *strNewGpsLongtitude = params.get(CameraParameters::KEY_GPS_LONGITUDE);
    if (m_secCamera->setGpsLongitude(strNewGpsLongtitude) == false) {
        ALOGE("ERR(%s):m_secCamera->setGpsLongitude(%s) fail", __func__, strNewGpsLongtitude);
        ret = UNKNOWN_ERROR;
    } else {
        if (strNewGpsLongtitude)
            m_params.set(CameraParameters::KEY_GPS_LONGITUDE, strNewGpsLongtitude);
        else
            m_params.remove(CameraParameters::KEY_GPS_LONGITUDE);
    }

    // gps processing method
    const char *strNewGpsProcessingMethod = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);

    if (m_secCamera->setGpsProcessingMethod(strNewGpsProcessingMethod) == false) {
        ALOGE("ERR(%s):m_secCamera->setGpsProcessingMethod(%s) fail", __func__, strNewGpsProcessingMethod);
        ret = UNKNOWN_ERROR;
    } else {
        if (strNewGpsProcessingMethod)
            m_params.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, strNewGpsProcessingMethod);
        else
            m_params.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    }

    // gps timestamp
    const char *strNewGpsTimestamp = params.get(CameraParameters::KEY_GPS_TIMESTAMP);
    if (m_secCamera->setGpsTimeStamp(strNewGpsTimestamp) == false) {
        ALOGE("ERR(%s):m_secCamera->setGpsTimeStamp(%s) fail", __func__, strNewGpsTimestamp);
        ret = UNKNOWN_ERROR;
    } else {
        if (strNewGpsTimestamp)
            m_params.set(CameraParameters::KEY_GPS_TIMESTAMP, strNewGpsTimestamp);
        else
            m_params.remove(CameraParameters::KEY_GPS_TIMESTAMP);
    }

    ///////////////////////////////////////////////////
    // Additional API.
    ///////////////////////////////////////////////////
    // brightness
    int newBrightness = params.getInt("brightness");
    int maxBrightness = params.getInt("brightness-max");
    int minBrightness = params.getInt("brightness-min");
    ALOGV("DEBUG(%s):newBrightness %d", __func__, newBrightness);
    if ((minBrightness <= newBrightness) && (newBrightness <= maxBrightness)) {
        if (m_secCamera->setBrightness(newBrightness) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setBrightness(%d)", __func__, newBrightness);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set("brightness", newBrightness);
        }
    }

    // saturation
    int newSaturation = params.getInt("saturation");
    int maxSaturation = params.getInt("saturation-max");
    int minSaturation = params.getInt("saturation-min");
    ALOGV("DEBUG(%s):newSaturation %d", __func__, newSaturation);
    if ((minSaturation <= newSaturation) && (newSaturation <= maxSaturation)) {
        if (m_secCamera->setSaturation(newSaturation) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setSaturation(%d)", __func__, newSaturation);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set("saturation", newSaturation);
        }
    }

    // sharpness
    int newSharpness = params.getInt("sharpness");
    int maxSharpness = params.getInt("sharpness-max");
    int minSharpness = params.getInt("sharpness-min");
    ALOGV("DEBUG(%s):newSharpness %d", __func__, newSharpness);
    if ((minSharpness <= newSharpness) && (newSharpness <= maxSharpness)) {
        if (m_secCamera->setSharpness(newSharpness) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setSharpness(%d)", __func__, newSharpness);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set("sharpness", newSharpness);
        }
    }

    // hue
    int newHue = params.getInt("hue");
    int maxHue = params.getInt("hue-max");
    int minHue = params.getInt("hue-min");
    ALOGV("DEBUG(%s):newHue %d", __func__, newHue);
    if ((minHue <= newHue) && (maxHue >= newHue)) {
        if (m_secCamera->setHue(newHue) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setHue(hue(%d))", __func__, newHue);
            ret = UNKNOWN_ERROR;
        } else {
            m_params.set("hue", newHue);
        }
    }

    // ISO
    const char *strNewISO = params.get("iso");
    ALOGV("DEBUG(%s):strNewISO %s", __func__, strNewISO);
    if (strNewISO != NULL) {
        int newISO = -1;

        if (!strcmp(strNewISO, "auto"))
            newISO = 0;
        else {
            newISO = (int)atoi(strNewISO);
            if (newISO == 0) {
                ALOGE("ERR(%s):Invalid iso value(%s)", __func__, strNewISO);
                ret = UNKNOWN_ERROR;
            }
        }

        if (0 <= newISO) {
            if (m_secCamera->setISO(newISO) == false) {
                ALOGE("ERR(%s):Fail on m_secCamera->setISO(iso(%d))", __func__, newISO);
                ret = UNKNOWN_ERROR;
            } else {
                m_params.set("iso", strNewISO);
            }
        }
    }

    //contrast
    const char *strNewContrast = params.get("contrast");
    ALOGV("DEBUG(%s):strNewContrast %s", __func__, strNewContrast);
    if (strNewContrast != NULL) {
        int newContrast = -1;

        if (!strcmp(strNewContrast, "auto"))
            newContrast = ExynosCamera::CONTRAST_AUTO;
        else if (!strcmp(strNewContrast, "-2"))
            newContrast = ExynosCamera::CONTRAST_MINUS_2;
        else if (!strcmp(strNewContrast, "-1"))
            newContrast = ExynosCamera::CONTRAST_MINUS_1;
        else if (!strcmp(strNewContrast, "0"))
            newContrast = ExynosCamera::CONTRAST_DEFAULT;
        else if (!strcmp(strNewContrast, "1"))
            newContrast = ExynosCamera::CONTRAST_PLUS_1;
        else if (!strcmp(strNewContrast, "2"))
            newContrast = ExynosCamera::CONTRAST_PLUS_2;
        else {
            ALOGE("ERR(%s):Invalid contrast value(%s)", __func__, strNewContrast);
            ret = UNKNOWN_ERROR;
        }

        if (0 <= newContrast) {
            if (m_secCamera->setContrast(newContrast) == false) {
                ALOGE("ERR(%s):Fail on m_secCamera->setContrast(contrast(%d))", __func__, newContrast);
                ret = UNKNOWN_ERROR;
            } else {
                m_params.set("contrast", strNewContrast);
            }
        }
    }

    //WDR
    int newWdr = params.getInt("wdr");
    ALOGV("DEBUG(%s):newWdr %d", __func__, newWdr);
    if (0 <= newWdr) {
        if (m_secCamera->setWDR(newWdr) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setWDR(%d)", __func__, newWdr);
            ret = UNKNOWN_ERROR;
        }
    }

    //anti shake
    int newAntiShake = m_internalParams.getInt("anti-shake");
    ALOGV("DEBUG(%s):newAntiShake %d", __func__, newAntiShake);
    if (0 <= newAntiShake) {
        bool toggle = false;
        if (newAntiShake == 1)
            toggle = true;

        if (m_secCamera->setAntiShake(toggle) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setAntiShake(%d)", __func__, newAntiShake);
            ret = UNKNOWN_ERROR;
        }
    }

    //gamma
    const char *strNewGamma = m_internalParams.get("video_recording_gamma");
    ALOGV("DEBUG(%s):strNewGamma %s", __func__, strNewGamma);
    if (strNewGamma != NULL) {
        int newGamma = -1;
        if (!strcmp(strNewGamma, "off"))
            newGamma = 0;
        else if (!strcmp(strNewGamma, "on"))
            newGamma = 1;
        else {
            ALOGE("ERR(%s):unmatched gamma(%s)", __func__, strNewGamma);
            ret = UNKNOWN_ERROR;
        }

        if (0 <= newGamma) {
            bool toggle = false;
            if (newGamma == 1)
                toggle = true;

            if (m_secCamera->setGamma(toggle) == false) {
                ALOGE("ERR(%s):m_secCamera->setGamma(%s) fail", __func__, strNewGamma);
                ret = UNKNOWN_ERROR;
            }
        }
    }

    //slow ae
    const char *strNewSlowAe = m_internalParams.get("slow_ae");
    ALOGV("DEBUG(%s):strNewSlowAe %s", __func__, strNewSlowAe);
    if (strNewSlowAe != NULL) {
        int newSlowAe = -1;

        if (!strcmp(strNewSlowAe, "off"))
            newSlowAe = 0;
        else if (!strcmp(strNewSlowAe, "on"))
            newSlowAe = 1;
        else {
            ALOGE("ERR(%s):unmatched slow_ae(%s)", __func__, strNewSlowAe);
            ret = UNKNOWN_ERROR;
        }

        if (0 <= newSlowAe) {
            bool toggle = false;
            if (newSlowAe == 1)
                toggle = true;
            if (m_secCamera->setSlowAE(newSlowAe) == false) {
                ALOGE("ERR(%s):m_secCamera->setSlowAE(%d) fail", __func__, newSlowAe);
                ret = UNKNOWN_ERROR;
            }
        }
    }

    // Shot mode
    int newShotMode = m_internalParams.getInt("shot_mode");
    ALOGV("DEBUG(%s):newShotMode %d", __func__, newShotMode);
    if (0 <= newShotMode) {
        if (m_secCamera->setShotMode(newShotMode) == false) {
            ALOGE("ERR(%s):Fail on m_secCamera->setShotMode(%d)", __func__, newShotMode);
            ret = UNKNOWN_ERROR;
        }
    } else {
        newShotMode=0;
    }

    ALOGV("DEBUG(%s):return ret = %d", __func__, ret);

    return ret;
}

CameraParameters ExynosCameraHWInterface::getParameters() const
{
    ALOGV("DEBUG(%s):", __func__);
    return m_params;
}

status_t ExynosCameraHWInterface::sendCommand(int32_t command, int32_t arg1, int32_t arg2)
{
    switch (command) {
    case CAMERA_CMD_START_FACE_DETECTION:
    case CAMERA_CMD_STOP_FACE_DETECTION:
        if (m_secCamera->getMaxNumDetectedFaces() == 0) {
            ALOGE("ERR(%s):getMaxNumDetectedFaces == 0", __func__);
            return BAD_VALUE;
        }

        if (arg1 == CAMERA_FACE_DETECTION_SW) {
            ALOGE("ERR(%s):only support HW face dectection", __func__);
            return BAD_VALUE;
        }

        if (command == CAMERA_CMD_START_FACE_DETECTION) {
            if (   m_secCamera->flagStartFaceDetection() == false
                && m_secCamera->startFaceDetection() == false) {
                ALOGE("ERR(%s):startFaceDetection() fail", __func__);
                return BAD_VALUE;
            }
        } else { // if (command == CAMERA_CMD_STOP_FACE_DETECTION)
            if (   m_secCamera->flagStartFaceDetection() == true
                && m_secCamera->stopFaceDetection() == false) {
                ALOGE("ERR(%s):stopFaceDetection() fail", __func__);
                return BAD_VALUE;
            }
        }
        break;
    default:
        ALOGE("ERR(%s):unexpectect command(%d) fail", __func__, command);
        return BAD_VALUE;
        break;
    }
    return NO_ERROR;
}

void ExynosCameraHWInterface::release()
{
    ALOGV("DEBUG(%s):", __func__);

    /* shut down any threads we have that might be running.  do it here
     * instead of the destructor.  we're guaranteed to be on another thread
     * than the ones below.  if we used the destructor, since the threads
     * have a reference to this object, we could wind up trying to wait
     * for ourself to exit, which is a deadlock.
     */
    if (m_videoThread != NULL) {
        m_videoThread->requestExit();
        m_exitVideoThread = true;
        m_videoRunning = true; // let it run so it can exit
        m_videoCondition.signal();
        m_videoThread->requestExitAndWait();
        m_videoThread.clear();
    }

    if (m_previewThread != NULL) {
        /* this thread is normally already in it's threadLoop but blocked
         * on the condition variable or running.  signal it so it wakes
         * up and can exit.
         */
        m_previewThread->requestExit();
        m_exitPreviewThread = true;
        m_previewRunning = true; // let it run so it can exit
        m_previewCondition.signal();
        m_previewThread->requestExitAndWait();
        m_previewThread.clear();
    }

    if (m_autoFocusThread != NULL) {
        /* this thread is normally already in it's threadLoop but blocked
         * on the condition variable.  signal it so it wakes up and can exit.
         */
        m_focusLock.lock();
        m_autoFocusThread->requestExit();
        m_exitAutoFocusThread = true;
        m_focusCondition.signal();
        m_focusLock.unlock();
        m_autoFocusThread->requestExitAndWait();
        m_autoFocusThread.clear();
    }

    if (m_pictureThread != NULL) {
        m_pictureThread->requestExitAndWait();
        m_pictureThread.clear();
    }

    for (int i = 0; i < NUM_OF_VIDEO_BUF; i++) {
        if (m_videoHeap[i]) {
            m_videoHeap[i]->release(m_videoHeap[i]);
            m_videoHeap[i] = 0;
        }

        if (m_resizedVideoHeap[i]) {
            m_resizedVideoHeap[i]->release(m_resizedVideoHeap[i]);
            m_resizedVideoHeap[i] = 0;
        }
    }

    for (int i = 0; i < NUM_OF_PREVIEW_BUF; i++) {
        if (m_previewHeap[i]) {
            m_previewHeap[i]->release(m_previewHeap[i]);
            m_previewHeap[i] = 0;
        }
    }

    if (m_pictureRunning == true) {
        if (m_stopPictureInternal() == false)
            ALOGE("ERR(%s):m_stopPictureInternal() fail", __func__);
    }

    if (m_exynosVideoCSC)
        csc_deinit(m_exynosVideoCSC);
    m_exynosVideoCSC = NULL;

    if (m_exynosPictureCSC)
        csc_deinit(m_exynosPictureCSC);
    m_exynosPictureCSC = NULL;

    if (m_exynosPreviewCSC)
        csc_deinit(m_exynosPreviewCSC);
    m_exynosPreviewCSC = NULL;

     /* close after all the heaps are cleared since those
     * could have dup'd our file descriptor.
     */
    if (m_secCamera->flagCreate() == true)
        m_secCamera->destroy();
}

status_t ExynosCameraHWInterface::dump(int fd) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    const Vector<String16> args;

    if (m_secCamera != 0) {
        m_params.dump(fd, args);
        m_internalParams.dump(fd, args);
        snprintf(buffer, 255, " preview running(%s)\n", m_previewRunning?"true": "false");
        result.append(buffer);
    } else {
        result.append("No camera client yet.\n");
    }

    write(fd, result.string(), result.size());
    return NO_ERROR;
}

int ExynosCameraHWInterface::getCameraId() const
{
    return m_secCamera->getCameraId();
}

void ExynosCameraHWInterface::m_initDefaultParameters(int cameraId)
{
    if (m_secCamera == NULL) {
        ALOGE("ERR(%s):m_secCamera object is NULL", __func__);
        return;
    }

    CameraParameters p;
    CameraParameters ip;

    String8 parameterString;

    char * cameraName;
    cameraName = m_secCamera->getCameraName();
    if (cameraName == NULL)
        ALOGE("ERR(%s):getCameraName() fail", __func__);

    /*
    if (cameraId == ExynosCamera::CAMERA_ID_BACK) {
        p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
              "3264x2448,2576x1948,1920x1080,1280x720,800x480,720x480,640x480,320x240,528x432,176x144");
        p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
              "3264x2448,1920x1080,1280x720,800x480,720x480,640x480");
        p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,
              "1920x1080,1280x720,640x480,176x144");
    } else {
        p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
              "1392x1392,1280x720,640x480,352x288,320x240,176x144");
        p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
              "1392x1392,1280x960,640x480");
        p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,
              "1280x720,640x480,176x144");
    }
    */

    char strBuf[256];
    String8 listString;

    // preview
    int previewMaxW  = 0;
    int previewMaxH  = 0;
    m_secCamera->getSupportedPreviewSizes(&previewMaxW, &previewMaxH);

    listString.setTo("");
    if (m_getResolutionList(listString, strBuf, previewMaxW, previewMaxH) == false) {
        ALOGE("ERR(%s):m_getResolutionList() fail", __func__);

        previewMaxW = 640;
        previewMaxH = 480;
        listString = String8::format("%dx%d", previewMaxW, previewMaxH);
    }

    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, listString.string());
    p.setPreviewSize(previewMaxW, previewMaxH);
    p.getSupportedPreviewSizes(m_supportedPreviewSizes);

    listString.setTo("");
    listString = String8::format("%s,%s", CameraParameters::PIXEL_FORMAT_YUV420SP, CameraParameters::PIXEL_FORMAT_YUV420P);
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, listString);
    p.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV420P);

    // video
    int videoMaxW = 0;
    int videoMaxH = 0;
    m_secCamera->getSupportedVideoSizes(&videoMaxW, &videoMaxH);

    listString.setTo("");
    if (m_getResolutionList(listString, strBuf, videoMaxW, videoMaxH) == false) {
        ALOGE("ERR(%s):m_getResolutionList() fail", __func__);

        videoMaxW = 640;
        videoMaxH = 480;
        listString = String8::format("%dx%d", videoMaxW, videoMaxH);
    }
    p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, listString.string());
    p.setVideoSize(videoMaxW, videoMaxH);

    int preferredPreviewW = 0;
    int preferredPreviewH = 0;
    m_secCamera->getPreferredPreivewSizeForVideo(&preferredPreviewW, &preferredPreviewH);
    listString.setTo("");
    listString = String8::format("%dx%d", preferredPreviewW, preferredPreviewH);
    p.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, listString.string());
    p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);

    if (m_secCamera->isVideoSnapshotSupported() == true)
        p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, "false");

    if (m_secCamera->isVideoStabilizationSupported() == true)
        p.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, "false");

    // picture
    int pictureMaxW = 0;
    int pictureMaxH = 0;
    m_secCamera->getSupportedPictureSizes(&pictureMaxW, &pictureMaxH);

    listString.setTo("");
    if (m_getResolutionList(listString, strBuf, pictureMaxW, pictureMaxH) == false) {
        ALOGE("ERR(%s):m_getResolutionList() fail", __func__);

        pictureMaxW = 640;
        pictureMaxW = 480;
        listString = String8::format("%dx%d", pictureMaxW, pictureMaxH);
    }
    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, listString.string());
    p.setPictureSize(pictureMaxW, pictureMaxH);

    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS,
          CameraParameters::PIXEL_FORMAT_JPEG);

    p.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);

    p.set(CameraParameters::KEY_JPEG_QUALITY, "100"); // maximum quality

    // thumbnail
    int thumbnailMaxW = 0;
    int thumbnailMaxH = 0;

    m_secCamera->getSupportedJpegThumbnailSizes(&thumbnailMaxW, &thumbnailMaxH);
    listString = String8::format("%dx%d", thumbnailMaxW, thumbnailMaxH);
    listString.append(",0x0");
    p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, listString.string());
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,  thumbnailMaxW);
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, thumbnailMaxH);
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "100");

    // exposure
    p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, m_secCamera->getMinExposureCompensation());
    p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, m_secCamera->getMaxExposureCompensation());
    p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, m_secCamera->getExposureCompensation());
    p.setFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, m_secCamera->getExposureCompensationStep());

    if (m_secCamera->isAutoExposureLockSupported() == true)
        p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "false");

    // face detection
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, m_secCamera->getMaxNumDetectedFaces());
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, 0);

    // focus mode
    int focusMode = m_secCamera->getSupportedFocusModes();
    parameterString.setTo("");
    if (focusMode & ExynosCamera::FOCUS_MODE_AUTO) {
        parameterString.append(CameraParameters::FOCUS_MODE_AUTO);
        parameterString.append(",");
    }
    if (focusMode & ExynosCamera::FOCUS_MODE_INFINITY) {
        parameterString.append(CameraParameters::FOCUS_MODE_INFINITY);
        parameterString.append(",");
    }
    if (focusMode & ExynosCamera::FOCUS_MODE_MACRO) {
        parameterString.append(CameraParameters::FOCUS_MODE_MACRO);
        parameterString.append(",");
    }
    if (focusMode & ExynosCamera::FOCUS_MODE_FIXED) {
        parameterString.append(CameraParameters::FOCUS_MODE_FIXED);
        parameterString.append(",");
    }
    if (focusMode & ExynosCamera::FOCUS_MODE_EDOF) {
        parameterString.append(CameraParameters::FOCUS_MODE_EDOF);
        parameterString.append(",");
    }
    if (focusMode & ExynosCamera::FOCUS_MODE_CONTINUOUS_VIDEO) {
        parameterString.append(CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO);
        parameterString.append(",");
    }
    if (focusMode & ExynosCamera::FOCUS_MODE_CONTINUOUS_PICTURE)
        parameterString.append(CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE);

    p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,
          parameterString.string());

    if (focusMode & ExynosCamera::FOCUS_MODE_CONTINUOUS_PICTURE)
        p.set(CameraParameters::KEY_FOCUS_MODE,
              CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE);
    else if (focusMode & ExynosCamera::FOCUS_MODE_CONTINUOUS_VIDEO)
        p.set(CameraParameters::KEY_FOCUS_MODE,
              CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO);
    else if (focusMode & ExynosCamera::FOCUS_MODE_AUTO)
        p.set(CameraParameters::KEY_FOCUS_MODE,
              CameraParameters::FOCUS_MODE_AUTO);
    else
        p.set(CameraParameters::KEY_FOCUS_MODE,
          CameraParameters::FOCUS_MODE_FIXED);

    // HACK
    if (cameraId == ExynosCamera::CAMERA_ID_BACK) {
        p.set(CameraParameters::KEY_FOCUS_DISTANCES,
              BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR);
        p.set(CameraParameters::FOCUS_DISTANCE_INFINITY,
              BACK_CAMERA_FOCUS_DISTANCE_INFINITY);
    } else {
        p.set(CameraParameters::KEY_FOCUS_DISTANCES,
              FRONT_CAMERA_FOCUS_DISTANCES_STR);
        p.set(CameraParameters::FOCUS_DISTANCE_INFINITY,
              FRONT_CAMERA_FOCUS_DISTANCE_INFINITY);
    }

    if (focusMode & ExynosCamera::FOCUS_MODE_TOUCH)
        p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, m_secCamera->getMaxNumFocusAreas());

    // flash
    int flashMode = m_secCamera->getSupportedFlashModes();
    parameterString.setTo("");
    if (flashMode & ExynosCamera::FLASH_MODE_OFF) {
        parameterString.append(CameraParameters::FLASH_MODE_OFF);
        parameterString.append(",");
    }
    if (flashMode & ExynosCamera::FLASH_MODE_AUTO) {
        parameterString.append(CameraParameters::FLASH_MODE_AUTO);
        parameterString.append(",");
    }
    if (flashMode & ExynosCamera::FLASH_MODE_ON) {
        parameterString.append(CameraParameters::FLASH_MODE_ON);
        parameterString.append(",");
    }
    if (flashMode & ExynosCamera::FLASH_MODE_RED_EYE) {
        parameterString.append(CameraParameters::FLASH_MODE_RED_EYE);
        parameterString.append(",");
    }
    if (flashMode & ExynosCamera::FLASH_MODE_TORCH)
        parameterString.append(CameraParameters::FLASH_MODE_TORCH);

    p.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, parameterString.string());
    p.set(CameraParameters::KEY_FLASH_MODE, CameraParameters::FLASH_MODE_OFF);

    // scene mode
    int sceneMode = m_secCamera->getSupportedSceneModes();
    parameterString.setTo("");
    if (sceneMode & ExynosCamera::SCENE_MODE_AUTO) {
        parameterString.append(CameraParameters::SCENE_MODE_AUTO);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_ACTION) {
        parameterString.append(CameraParameters::SCENE_MODE_ACTION);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_PORTRAIT) {
        parameterString.append(CameraParameters::SCENE_MODE_PORTRAIT);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_LANDSCAPE) {
        parameterString.append(CameraParameters::SCENE_MODE_LANDSCAPE);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_NIGHT) {
        parameterString.append(CameraParameters::SCENE_MODE_NIGHT);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_NIGHT_PORTRAIT) {
        parameterString.append(CameraParameters::SCENE_MODE_NIGHT_PORTRAIT);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_THEATRE) {
        parameterString.append(CameraParameters::SCENE_MODE_THEATRE);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_BEACH) {
        parameterString.append(CameraParameters::SCENE_MODE_BEACH);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_SNOW) {
        parameterString.append(CameraParameters::SCENE_MODE_SNOW);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_SUNSET) {
        parameterString.append(CameraParameters::SCENE_MODE_SUNSET);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_STEADYPHOTO) {
        parameterString.append(CameraParameters::SCENE_MODE_STEADYPHOTO);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_FIREWORKS) {
        parameterString.append(CameraParameters::SCENE_MODE_FIREWORKS);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_SPORTS) {
        parameterString.append(CameraParameters::SCENE_MODE_SPORTS);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_PARTY) {
        parameterString.append(CameraParameters::SCENE_MODE_PARTY);
        parameterString.append(",");
    }
    if (sceneMode & ExynosCamera::SCENE_MODE_CANDLELIGHT)
        parameterString.append(CameraParameters::SCENE_MODE_CANDLELIGHT);

    p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES,
          parameterString.string());
    p.set(CameraParameters::KEY_SCENE_MODE,
          CameraParameters::SCENE_MODE_AUTO);

    // effect
    int effect = m_secCamera->getSupportedColorEffects();
    parameterString.setTo("");
    if (effect & ExynosCamera::EFFECT_NONE) {
        parameterString.append(CameraParameters::EFFECT_NONE);
        parameterString.append(",");
    }
    if (effect & ExynosCamera::EFFECT_MONO) {
        parameterString.append(CameraParameters::EFFECT_MONO);
        parameterString.append(",");
    }
    if (effect & ExynosCamera::EFFECT_NEGATIVE) {
        parameterString.append(CameraParameters::EFFECT_NEGATIVE);
        parameterString.append(",");
    }
    if (effect & ExynosCamera::EFFECT_SOLARIZE) {
        parameterString.append(CameraParameters::EFFECT_SOLARIZE);
        parameterString.append(",");
    }
    if (effect & ExynosCamera::EFFECT_SEPIA) {
        parameterString.append(CameraParameters::EFFECT_SEPIA);
        parameterString.append(",");
    }
    if (effect & ExynosCamera::EFFECT_POSTERIZE) {
        parameterString.append(CameraParameters::EFFECT_POSTERIZE);
        parameterString.append(",");
    }
    if (effect & ExynosCamera::EFFECT_WHITEBOARD) {
        parameterString.append(CameraParameters::EFFECT_WHITEBOARD);
        parameterString.append(",");
    }
    if (effect & ExynosCamera::EFFECT_BLACKBOARD) {
        parameterString.append(CameraParameters::EFFECT_BLACKBOARD);
        parameterString.append(",");
    }
    if (effect & ExynosCamera::EFFECT_AQUA)
        parameterString.append(CameraParameters::EFFECT_AQUA);

    p.set(CameraParameters::KEY_SUPPORTED_EFFECTS, parameterString.string());
    p.set(CameraParameters::KEY_EFFECT, CameraParameters::EFFECT_NONE);

    // white balance
    int whiteBalance = m_secCamera->getSupportedWhiteBalance();
    parameterString.setTo("");
    if (whiteBalance & ExynosCamera::WHITE_BALANCE_AUTO) {
        parameterString.append(CameraParameters::WHITE_BALANCE_AUTO);
        parameterString.append(",");
    }
    if (whiteBalance & ExynosCamera::WHITE_BALANCE_INCANDESCENT) {
        parameterString.append(CameraParameters::WHITE_BALANCE_INCANDESCENT);
        parameterString.append(",");
    }
    if (whiteBalance & ExynosCamera::WHITE_BALANCE_FLUORESCENT) {
        parameterString.append(CameraParameters::WHITE_BALANCE_FLUORESCENT);
        parameterString.append(",");
    }
    if (whiteBalance & ExynosCamera::WHITE_BALANCE_WARM_FLUORESCENT) {
        parameterString.append(CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT);
        parameterString.append(",");
    }
    if (whiteBalance & ExynosCamera::WHITE_BALANCE_DAYLIGHT) {
        parameterString.append(CameraParameters::WHITE_BALANCE_DAYLIGHT);
        parameterString.append(",");
    }
    if (whiteBalance & ExynosCamera::WHITE_BALANCE_CLOUDY_DAYLIGHT) {
        parameterString.append(CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT);
        parameterString.append(",");
    }
    if (whiteBalance & ExynosCamera::WHITE_BALANCE_TWILIGHT) {
        parameterString.append(CameraParameters::WHITE_BALANCE_TWILIGHT);
        parameterString.append(",");
    }
    if (whiteBalance & ExynosCamera::WHITE_BALANCE_SHADE)
        parameterString.append(CameraParameters::WHITE_BALANCE_SHADE);

    p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE,
          parameterString.string());
    p.set(CameraParameters::KEY_WHITE_BALANCE, CameraParameters::WHITE_BALANCE_AUTO);

    if (m_secCamera->isAutoWhiteBalanceLockSupported() == true)
        p.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "false");

    // anti banding
    int antiBanding = m_secCamera->getSupportedAntibanding();
    parameterString.setTo("");
    if (antiBanding & ExynosCamera::ANTIBANDING_AUTO) {
        parameterString.append(CameraParameters::ANTIBANDING_AUTO);
        parameterString.append(",");
    }
    if (antiBanding & ExynosCamera::ANTIBANDING_50HZ) {
        parameterString.append(CameraParameters::ANTIBANDING_50HZ);
        parameterString.append(",");
    }
    if (antiBanding & ExynosCamera::ANTIBANDING_60HZ) {
        parameterString.append(CameraParameters::ANTIBANDING_60HZ);
        parameterString.append(",");
    }
    if (antiBanding & ExynosCamera::ANTIBANDING_OFF)
        parameterString.append(CameraParameters::ANTIBANDING_OFF);

    p.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING,
          parameterString.string());

    p.set(CameraParameters::KEY_ANTIBANDING, CameraParameters::ANTIBANDING_OFF);

    // rotation
    p.set(CameraParameters::KEY_ROTATION, 0);

    // view angle
    p.setFloat(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, m_secCamera->getHorizontalViewAngle());
    p.setFloat(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, m_secCamera->getVerticalViewAngle());

    // metering
    if (0 < m_secCamera->getMaxNumMeteringAreas())
        p.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS, m_secCamera->getMaxNumMeteringAreas());

    // zoom
    if (m_secCamera->isZoomSupported() == true) {

        int maxZoom = m_secCamera->getMaxZoom();
        if (0 < maxZoom) {

            p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "true");

            if (m_secCamera->isSmoothZoomSupported() == true)
                p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "true");
            else
                p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");

            p.set(CameraParameters::KEY_MAX_ZOOM,            maxZoom);
            p.set(CameraParameters::KEY_ZOOM,                m_secCamera->getZoom());

            int max_zoom_ratio = m_secCamera->getMaxZoomRatio();

            listString.setTo("");

            if (m_getZoomRatioList(listString, strBuf, maxZoom, 100, max_zoom_ratio) == true)
                p.set(CameraParameters::KEY_ZOOM_RATIOS, listString.string());
            else
                p.set(CameraParameters::KEY_ZOOM_RATIOS, "100");
        } else {
            p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
            p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
        }

    } else {
        p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
        p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
    }

    // fps
    int minPreviewFps, maxPreviewFps;
    m_secCamera->getPreviewFpsRange(&minPreviewFps, &maxPreviewFps);

    int baseFps = ((minPreviewFps + 5) / 5) * 5;

    listString.setTo("");
    snprintf(strBuf, 256, "%d", minPreviewFps);
    listString.append(strBuf);

    for (int i = baseFps; i <= maxPreviewFps; i += 5) {
        int step = (i / 5) * 5;
        snprintf(strBuf, 256, ",%d", step);
        listString.append(strBuf);
    }
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,  listString.string());
    p.setPreviewFrameRate(maxPreviewFps);

    int minFpsRange = minPreviewFps * 1000; // 15 -> 15000
    int maxFpsRange = maxPreviewFps * 1000; // 30 -> 30000

    snprintf(strBuf, 256, "(%d,%d)", minFpsRange, maxFpsRange);
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, strBuf);

    snprintf(strBuf, 256, "%d,%d", minFpsRange, maxFpsRange);
    p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, strBuf);
    //p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(15000,30000)");
    //p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "15000,30000")

    // focal length
    int num = 0;
    int den = 0;
    int precision = 0;
    m_secCamera->getFocalLength(&num, &den);

    switch (den) {
    default:
    case 1000:
        precision = 3;
        break;
    case 100:
        precision = 2;
        break;
    case 10:
        precision = 1;
        break;
    case 1:
        precision = 0;
        break;
    }

    snprintf(strBuf, 256, "%.*f", precision, ((float)num / (float)den));
    p.set(CameraParameters::KEY_FOCAL_LENGTH, strBuf);
    //p.set(CameraParameters::KEY_FOCAL_LENGTH, "3.43");
    //p.set(CameraParameters::KEY_FOCAL_LENGTH, "0.9");

    // Additional params.

    p.set("contrast", "auto");
    p.set("iso", "auto");
    p.set("wdr", 0);
    p.set("metering", "center");

    p.set("brightness", 0);
    p.set("brightness-max", 2);
    p.set("brightness-min", -2);

    p.set("saturation", 0);
    p.set("saturation-max", 2);
    p.set("saturation-min", -2);

    p.set("sharpness", 0);
    p.set("sharpness-max", 2);
    p.set("sharpness-min", -2);

    p.set("hue", 0);
    p.set("hue-max", 2);
    p.set("hue-min", -2);

    m_params = p;
    m_internalParams = ip;

    /* make sure m_secCamera has all the settings we do.  applications
     * aren't required to call setParameters themselves (only if they
     * want to change something.
     */
    setParameters(p);

    m_secCamera->setPreviewFrameRate(maxPreviewFps);
}

bool ExynosCameraHWInterface::m_startPreviewInternal(void)
{
    ALOGV("DEBUG(%s):", __func__);

    int i;
    int previewW, previewH, previewFormat, previewFramesize;

    m_secCamera->getPreviewSize(&previewW, &previewH);
    previewFormat = m_secCamera->getPreviewFormat();

    // we will use previewFramesize for m_previewHeap[i]
    previewFramesize = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(m_orgPreviewRect.colorFormat), m_orgPreviewRect.w, m_orgPreviewRect.h);

    ExynosBuffer previewBuf;
    void *virtAddr[3];
    int fd[3];

    for (i = 0; i < 3; i++) {
        virtAddr[i] = NULL;
	fd[i] = -1;
    }

    for (i = 0; i < NUM_OF_PREVIEW_BUF; i++) {

        m_avaliblePreviewBufHandle[i] = false;

        if (m_previewWindow->dequeue_buffer(m_previewWindow, &m_previewBufHandle[i], &m_previewStride[i]) != 0) {
            ALOGE("ERR(%s):Could not dequeue gralloc buffer[%d]!!", __func__, i);
            continue;
        } else {
             if (m_previewWindow->lock_buffer(m_previewWindow, m_previewBufHandle[i]) != 0)
                 ALOGE("ERR(%s):Could not lock gralloc buffer[%d]!!", __func__, i);
        }

	if (m_flagGrallocLocked[i] == false) {
            if (m_grallocHal->lock(m_grallocHal,
				   *m_previewBufHandle[i],
				   GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_YUV_ADDR,
                                   0, 0, previewW, previewH, virtAddr) != 0) {
                ALOGE("ERR(%s):could not obtain gralloc buffer", __func__);

                if (m_previewWindow->cancel_buffer(m_previewWindow, m_previewBufHandle[i]) != 0)
                    ALOGE("ERR(%s):Could not cancel_buffer gralloc buffer[%d]!!", __func__, i);

                continue;
            }

	    const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*m_previewBufHandle[i]);
	    fd[0] = priv_handle->fd;
	    fd[1] = priv_handle->u_fd;
	    fd[2] = priv_handle->v_fd;
            m_grallocVirtAddr[i] = virtAddr[0];
            m_matchedGrallocIndex[i] = i;
            m_flagGrallocLocked[i] = true;
        }

        m_getAlignedYUVSize(previewFormat, previewW, previewH, &previewBuf);

        previewBuf.reserved.p = i;
        previewBuf.virt.extP[0] = (char *)virtAddr[0];
        previewBuf.virt.extP[1] = (char *)virtAddr[1];
        previewBuf.virt.extP[2] = (char *)virtAddr[2];
	previewBuf.fd.extFd[0] = fd[0];
	previewBuf.fd.extFd[1] = fd[1];
	previewBuf.fd.extFd[2] = fd[2];

        m_secCamera->setPreviewBuf(&previewBuf);

        if (m_previewHeap[i]) {
            m_previewHeap[i]->release(m_previewHeap[i]);
            m_previewHeap[i] = 0;
        }

        m_previewHeap[i] = m_getMemoryCb(-1, previewFramesize, 1, 0);
        if (!m_previewHeap[i]) {
            ALOGE("ERR(%s):m_getMemoryCb(m_previewHeap[%d], size(%d) fail", __func__, i, previewFramesize);
            continue;
        }

        m_avaliblePreviewBufHandle[i] = true;
    }

    if (m_secCamera->startPreview() == false) {
        ALOGE("ERR(%s):Fail on m_secCamera->startPreview()", __func__);
        return false;
    }

    for (i = NUM_OF_PREVIEW_BUF - m_minUndequeuedBufs; i < NUM_OF_PREVIEW_BUF; i++) {
        if (m_secCamera->getPreviewBuf(&previewBuf) == false) {
            ALOGE("ERR(%s):getPreviewBuf() fail", __func__);
            return false;
        }

        if (m_grallocHal && m_flagGrallocLocked[previewBuf.reserved.p] == true) {
            m_grallocHal->unlock(m_grallocHal, *m_previewBufHandle[previewBuf.reserved.p]);
            m_flagGrallocLocked[previewBuf.reserved.p] = false;
        }

        if (m_previewWindow->cancel_buffer(m_previewWindow, m_previewBufHandle[previewBuf.reserved.p]) != 0)
            ALOGE("ERR(%s):Could not cancel_buffer gralloc buffer[%d]!!", __func__, previewBuf.reserved.p);

        m_avaliblePreviewBufHandle[previewBuf.reserved.p] = false;
    }

    m_setSkipFrame(INITIAL_SKIP_FRAME);

    if (m_pictureRunning == false
        && m_startPictureInternal() == false)
        ALOGE("ERR(%s):m_startPictureInternal() fail", __func__);

    return true;
}

void ExynosCameraHWInterface::m_stopPreviewInternal(void)
{
    ALOGV("DEBUG(%s):", __func__);

    /* request that the preview thread stop. */
    if (m_previewRunning == true) {
        m_previewRunning = false;

        if (m_previewStartDeferred == false) {
            m_previewCondition.signal();
            /* wait until preview thread is stopped */
            m_previewStoppedCondition.wait(m_previewLock);

            for (int i = 0; i < NUM_OF_PREVIEW_BUF; i++) {
                if (m_previewBufHandle[i] != NULL) {
                    if (m_grallocHal && m_flagGrallocLocked[i] == true) {
                        m_grallocHal->unlock(m_grallocHal, *m_previewBufHandle[i]);
                        m_flagGrallocLocked[i] = false;
                    }

                    if (m_avaliblePreviewBufHandle[i] == true) {
                        if (m_previewWindow->cancel_buffer(m_previewWindow, m_previewBufHandle[i]) != 0) {
                            ALOGE("ERR(%s):Fail to cancel buffer(%d)", __func__, i);
                        } else {
                            m_previewBufHandle[i] = NULL;
                            m_previewStride[i] = NULL;
                        }

                        m_avaliblePreviewBufHandle[i] = false;
                    }
                }
            }
        } else {
            ALOGV("DEBUG(%s):preview running but deferred, doing nothing", __func__);
        }
    } else {
        ALOGV("DEBUG(%s):preview not running, doing nothing", __func__);
    }
}

bool ExynosCameraHWInterface::m_previewThreadFuncWrapper(void)
{
    ALOGV("DEBUG(%s):starting", __func__);
    while (1) {
        m_previewLock.lock();
        while (m_previewRunning == false) {
            if (   m_secCamera->flagStartPreview() == true
                && m_secCamera->stopPreview() == false)
                ALOGE("ERR(%s):Fail on m_secCamera->stopPreview()", __func__);

            ALOGV("DEBUG(%s):calling m_secCamera->stopPreview() and waiting", __func__);

            m_previewStoppedCondition.signal();
            m_previewCondition.wait(m_previewLock);
            ALOGV("DEBUG(%s):return from wait", __func__);
        }
        m_previewLock.unlock();

        if (m_exitPreviewThread == true) {
            if (   m_secCamera->flagStartPreview() == true
                && m_secCamera->stopPreview() == false)
                ALOGE("ERR(%s):Fail on m_secCamera->stopPreview()", __func__);

            return true;
        }
        m_previewThreadFunc();
    }
}

bool ExynosCameraHWInterface::m_previewThreadFunc(void)
{
    ExynosBuffer previewBuf, callbackBuf;
    int stride;
    int previewW, previewH;
    bool doPutPreviewBuf = true;

    if (m_secCamera->getPreviewBuf(&previewBuf) == false) {
        ALOGE("ERR(%s):getPreviewBuf() fail", __func__);
        return false;
    }

#ifndef USE_3DNR_DMAOUT
    if (m_videoStart == true) {
        copy_previewBuf = previewBuf;
        m_videoRunning = true;
        m_videoCondition.signal();
    }
#endif

    m_skipFrameLock.lock();
    if (0 < m_skipFrame) {
        m_skipFrame--;
        m_skipFrameLock.unlock();
        ALOGV("DEBUG(%s):skipping %d frame", __func__, previewBuf.reserved.p);

        if (   doPutPreviewBuf == true
            && m_secCamera->putPreviewBuf(&previewBuf) == false) {
            ALOGE("ERR(%s):putPreviewBuf(%d) fail", __func__, previewBuf.reserved.p);
            return false;
        }

        return true;
    }
    m_skipFrameLock.unlock();

    callbackBuf = previewBuf;

    m_secCamera->getPreviewSize(&previewW, &previewH);

    if (m_previewWindow && m_grallocHal && m_previewRunning == true) {

        bool findGrallocBuf = false;
        buffer_handle_t *bufHandle = NULL;
        void *virtAddr[3];
	int fd[3];

        /* Unlock grallocHal buffer if locked */
        if (m_flagGrallocLocked[previewBuf.reserved.p] == true) {
            m_grallocHal->unlock(m_grallocHal, *m_previewBufHandle[previewBuf.reserved.p]);
            m_flagGrallocLocked[previewBuf.reserved.p] = false;
        } else {
            if (m_previewWindow->lock_buffer(m_previewWindow, bufHandle) != 0)
                ALOGE("ERR(%s):Could not lock gralloc buffer!!", __func__);
        }

        /* Enqueue lastest buffer */
        if (m_avaliblePreviewBufHandle[previewBuf.reserved.p] == true) {
            if (m_previewWindow->enqueue_buffer(m_previewWindow,
                                                m_previewBufHandle[previewBuf.reserved.p]) != 0) {
                ALOGE("ERR(%s):Could not enqueue gralloc buffer[%d]!!", __func__, previewBuf.reserved.p);
                goto callbacks;
            }

            m_avaliblePreviewBufHandle[previewBuf.reserved.p] = false;
        }

        /* Dequeue buffer from Gralloc */
        if (m_previewWindow->dequeue_buffer(m_previewWindow,
                                            &bufHandle,
                                            &stride) != 0) {
            ALOGE("ERR(%s):Could not dequeue gralloc buffer!!", __func__);
            goto callbacks;
        }

        /* Get virtual address from dequeued buf */
        if (m_grallocHal->lock(m_grallocHal,
                               *bufHandle,
                               GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_YUV_ADDR,
                               0, 0, previewW, previewH, virtAddr) != 0) {
            ALOGE("ERR(%s):could not obtain gralloc buffer", __func__);
            goto callbacks;
        }

	const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*bufHandle);
	fd[0] = priv_handle->fd;
	fd[1] = priv_handle->u_fd;
	fd[2] = priv_handle->v_fd;

        for (int i = 0; i < NUM_OF_PREVIEW_BUF; i++) {
            if ((unsigned int)m_grallocVirtAddr[i] == (unsigned int)virtAddr[0]) {
                findGrallocBuf = true;

                m_previewBufHandle[i] = bufHandle;
                m_previewStride[i] = stride;

                previewBuf.reserved.p = i;
                previewBuf.virt.extP[0] = (char *)virtAddr[0];
                previewBuf.virt.extP[1] = (char *)virtAddr[1];
                previewBuf.virt.extP[2] = (char *)virtAddr[2];

                previewBuf.fd.extFd[0] = fd[0];
                previewBuf.fd.extFd[1] = fd[1];
                previewBuf.fd.extFd[2] = fd[2];

                m_secCamera->setPreviewBuf(&previewBuf);
                m_matchedGrallocIndex[previewBuf.reserved.p] = i;
                m_avaliblePreviewBufHandle[i] = true;
                break;
            }
        }

        if (findGrallocBuf == false) {
            ALOGE("%s:addr(%x) is not matched any gralloc buffer's addr", __func__, virtAddr[0]);
            goto callbacks;
        }

        if (   doPutPreviewBuf == true
            && m_secCamera->putPreviewBuf(&previewBuf) == false)
            ALOGE("ERR(%s):putPreviewBuf(%d) fail", __func__, previewBuf.reserved.p);
        else
            doPutPreviewBuf = false;
    }

callbacks:

    if (   m_previewRunning == true
        && m_msgEnabled & CAMERA_MSG_PREVIEW_FRAME) {

        // resize from previewBuf(max size) to m_previewHeap(user's set size)
        if (m_exynosPreviewCSC) {
            int previewFormat = m_secCamera->getPreviewFormat();

            csc_set_src_format(m_exynosPreviewCSC,
                               previewW, previewH - 8,
                               0, 0, previewW, previewH - 8,
                               V4L2_PIX_2_HAL_PIXEL_FORMAT(previewFormat),
                               0);

            csc_set_dst_format(m_exynosPreviewCSC,
                               m_orgPreviewRect.w, m_orgPreviewRect.h,
                               0, 0, m_orgPreviewRect.w, m_orgPreviewRect.h,
                               V4L2_PIX_2_HAL_PIXEL_FORMAT(m_orgPreviewRect.colorFormat),
                               1);


            csc_set_src_buffer(m_exynosPreviewCSC,
                              (unsigned char *)callbackBuf.virt.extP[0],
                              (unsigned char *)callbackBuf.virt.extP[1],
                              (unsigned char *)callbackBuf.virt.extP[2],
                               0);

            ExynosBuffer dstBuf;
            m_getAlignedYUVSize(m_orgPreviewRect.colorFormat, m_orgPreviewRect.w, m_orgPreviewRect.h, &dstBuf);

            dstBuf.virt.extP[0] = (char *)m_previewHeap[callbackBuf.reserved.p]->data;
            for (int i = 1; i < 3; i++) {
                if (dstBuf.size.extS[i] != 0)
                    dstBuf.virt.extP[i] = dstBuf.virt.extP[i-1] + dstBuf.size.extS[i-1];
            }

            csc_set_dst_buffer(m_exynosPreviewCSC,
                              (unsigned char *)dstBuf.virt.extP[0],
                              (unsigned char *)dstBuf.virt.extP[1],
                              (unsigned char *)dstBuf.virt.extP[2],
                              0);

            if (csc_convert(m_exynosPreviewCSC) != 0)
                ALOGE("ERR(%s):csc_convert() fail", __func__);
        } else {
            ALOGE("ERR(%s):m_exynosPreviewCSC == NULL", __func__);
        }
    }

    /* TODO: We need better error handling scheme than this scheme */
    if (   doPutPreviewBuf == true
        && m_secCamera->putPreviewBuf(&previewBuf) == false)
        ALOGE("ERR(%s):putPreviewBuf(%d) fail", __func__, previewBuf.reserved.p);
    else
        doPutPreviewBuf = false;

    if (   m_previewRunning == true
        && m_msgEnabled & CAMERA_MSG_PREVIEW_FRAME) {
        m_dataCb(CAMERA_MSG_PREVIEW_FRAME, m_previewHeap[callbackBuf.reserved.p], 0, NULL, m_callbackCookie);
    }

    /* Face detection */
    if (   m_previewRunning == true
        && m_msgEnabled & CAMERA_MSG_PREVIEW_METADATA
        && m_secCamera->flagStartFaceDetection() == true) {

        camera_frame_metadata_t *ptrMetadata = NULL;

        int id[NUM_OF_DETECTED_FACES];
        int score[NUM_OF_DETECTED_FACES];
        ExynosRect2 detectedFace[NUM_OF_DETECTED_FACES];
        ExynosRect2 detectedLeftEye[NUM_OF_DETECTED_FACES];
        ExynosRect2 detectedRightEye[NUM_OF_DETECTED_FACES];
        ExynosRect2 detectedMouth[NUM_OF_DETECTED_FACES];

        int numOfDetectedFaces = m_secCamera->getDetectedFacesAreas(NUM_OF_DETECTED_FACES,
                                                  id,
                                                  score,
                                                  detectedFace,
                                                  detectedLeftEye,
                                                  detectedRightEye,
                                                  detectedMouth);

        if (0 < numOfDetectedFaces) {
            // camera.h
            // width   : -1000~1000
            // height  : -1000~1000
            // if eye, mouth is not detectable : -2000, -2000.

            int realNumOfDetectedFaces = 0;
            m_faceDetected = true;

            for (int i = 0; i < numOfDetectedFaces; i++) {
                // over 50s, we will catch
                //if (score[i] < 50)
                //    continue;

                m_faces[realNumOfDetectedFaces].rect[0] = m_calibratePosition(previewW, 2000, detectedFace[i].x1) - 1000;
                m_faces[realNumOfDetectedFaces].rect[1] = m_calibratePosition(previewH, 2000, detectedFace[i].y1) - 1000;
                m_faces[realNumOfDetectedFaces].rect[2] = m_calibratePosition(previewW, 2000, detectedFace[i].x2) - 1000;
                m_faces[realNumOfDetectedFaces].rect[3] = m_calibratePosition(previewH, 2000, detectedFace[i].y2) - 1000;

                m_faces[realNumOfDetectedFaces].id = id[i];
                m_faces[realNumOfDetectedFaces].score = score[i];

                m_faces[realNumOfDetectedFaces].left_eye[0] = (detectedLeftEye[i].x1 < 0) ? -2000 : m_calibratePosition(previewW, 2000, detectedLeftEye[i].x1) - 1000;
                m_faces[realNumOfDetectedFaces].left_eye[1] = (detectedLeftEye[i].y1 < 0) ? -2000 : m_calibratePosition(previewH, 2000, detectedLeftEye[i].y1) - 1000;

                m_faces[realNumOfDetectedFaces].right_eye[0] = (detectedRightEye[i].x1 < 0) ? -2000 : m_calibratePosition(previewW, 2000, detectedRightEye[i].x1) - 1000;
                m_faces[realNumOfDetectedFaces].right_eye[1] = (detectedRightEye[i].y1 < 0) ? -2000 : m_calibratePosition(previewH, 2000, detectedRightEye[i].y1) - 1000;

                m_faces[realNumOfDetectedFaces].mouth[0] = (detectedMouth[i].x1 < 0) ? -2000 : m_calibratePosition(previewW, 2000, detectedMouth[i].x1) - 1000;
                m_faces[realNumOfDetectedFaces].mouth[1] = (detectedMouth[i].y1 < 0) ? -2000 : m_calibratePosition(previewH, 2000, detectedMouth[i].y1) - 1000;

                realNumOfDetectedFaces++;
            }

            m_frameMetadata.number_of_faces = realNumOfDetectedFaces;
            m_frameMetadata.faces = m_faces;

            ptrMetadata = &m_frameMetadata;

            m_dataCb(CAMERA_MSG_PREVIEW_METADATA, m_previewHeap[callbackBuf.reserved.p], 0, ptrMetadata, m_callbackCookie);
        } else if (numOfDetectedFaces == 0 && m_faceDetected == true) {
            m_frameMetadata.number_of_faces = 0;
            m_frameMetadata.faces = m_faces;

            ptrMetadata = &m_frameMetadata;

            m_dataCb(CAMERA_MSG_PREVIEW_METADATA, m_previewHeap[callbackBuf.reserved.p], 0, ptrMetadata, m_callbackCookie);
            m_faceDetected = false;
        }
    }

    // zero shutter lag
    if (m_pictureRunning == false
        && m_startPictureInternal() == false)
        ALOGE("ERR(%s):m_startPictureInternal() fail", __func__);

    m_stateLock.lock();
    if (m_captureInProgress == true) {
        m_stateLock.unlock();
    } else {
        m_stateLock.unlock();

        if (m_numOfAvaliblePictureBuf < NUM_OF_PICTURE_BUF) {

            ExynosBufferQueue *cur = m_oldPictureBufQueueHead;
            do {
                if(cur->next == NULL) {
                    cur->buf = m_pictureBuf;
                    break;
                }
                cur = cur->next;
            } while (cur->next);

            if (m_secCamera->getPictureBuf(&m_pictureBuf) == false)
                ALOGE("ERR(%s):getPictureBuf() fail", __func__);
            else
                m_numOfAvaliblePictureBuf++;
        }

        if (NUM_OF_WAITING_PUT_PICTURE_BUF < m_numOfAvaliblePictureBuf) {
            ExynosBuffer nullBuf;
            ExynosBuffer oldBuf;

            oldBuf = m_oldPictureBufQueueHead->buf;

            m_oldPictureBufQueueHead->buf  = nullBuf;

            if (m_oldPictureBufQueueHead->next) {
                ExynosBufferQueue *newQueueHead = m_oldPictureBufQueueHead->next;
                m_oldPictureBufQueueHead->next = NULL;
                m_oldPictureBufQueueHead = newQueueHead;
            } else {
                m_oldPictureBufQueueHead = &m_oldPictureBufQueue[0];
            }

            if (oldBuf != nullBuf) {
                if (m_secCamera->putPictureBuf(&oldBuf) == false)
                    ALOGE("ERR(%s):putPictureBuf(%d) fail", __func__, oldBuf.reserved.p);
                else {
                    m_numOfAvaliblePictureBuf--;
                    if (m_numOfAvaliblePictureBuf < 0)
                        m_numOfAvaliblePictureBuf = 0;
                }

            }
        }
    }

    return true;
}

bool ExynosCameraHWInterface::m_videoThreadFuncWrapper(void)
{
    while (1) {
        while (m_videoRunning == false) {
            m_videoLock.lock();

#ifdef USE_3DNR_DMAOUT
            if (   m_secCamera->flagStartVideo() == true
                && m_secCamera->stopVideo() == false)
                ALOGE("ERR(%s):Fail on m_secCamera->stopVideo()", __func__);
#endif

            ALOGV("DEBUG(%s):calling mExynosCamera->stopVideo() and waiting", __func__);

            m_videoStoppedCondition.signal();
            m_videoCondition.wait(m_videoLock);
            ALOGV("DEBUG(%s):return from wait", __func__);

            m_videoLock.unlock();
        }

        if (m_exitVideoThread == true) {
            m_videoLock.lock();

#ifdef USE_3DNR_DMAOUT
            if (   m_secCamera->flagStartVideo() == true
                && m_secCamera->stopVideo() == false)
                ALOGE("ERR(%s):Fail on m_secCamera->stopVideo()", __func__);
#endif

            m_videoLock.unlock();
            return true;
        }

        m_videoThreadFunc();
#ifndef USE_3DNR_DMAOUT
        m_videoRunning = false;
#endif
    }
    return true;
}

bool ExynosCameraHWInterface::m_videoThreadFunc(void)
{
    nsecs_t timestamp;
#ifdef USE_3DNR_DMAOUT
    ExynosBuffer videoBuf;
#endif

    if (m_numOfAvailableVideoBuf == 0)
        usleep(1000); // sleep 1msec for other threads.

    {
        if (   m_msgEnabled & CAMERA_MSG_VIDEO_FRAME
            && m_videoRunning == true) {

            Mutex::Autolock lock(m_videoLock);

            if (m_numOfAvailableVideoBuf == 0) {
                ALOGV("DEBUG(%s):waiting releaseRecordingFrame()", __func__);
                return true;
            }

#ifdef USE_3DNR_DMAOUT
            if (m_secCamera->getVideoBuf(&videoBuf) == false) {
                ALOGE("ERR(%s):Fail on ExynosCamera->getVideoBuf()", __func__);
                return false;
            }
#endif

            m_numOfAvailableVideoBuf--;
            if (m_numOfAvailableVideoBuf < 0)
                m_numOfAvailableVideoBuf = 0;

            timestamp = systemTime(SYSTEM_TIME_MONOTONIC);

            // Notify the client of a new frame.
            if (   m_msgEnabled & CAMERA_MSG_VIDEO_FRAME
                && m_videoRunning == true) {

                // resize from videoBuf(max size) to m_videoHeap(user's set size)
                if (m_exynosVideoCSC) {
                    int videoW, videoH, videoFormat = 0;
                    int cropX, cropY, cropW, cropH = 0;

#ifndef USE_3DNR_DMAOUT
                    int previewW, previewH, previewFormat = 0;
                    previewFormat = m_secCamera->getPreviewFormat();
                    m_secCamera->getPreviewSize(&previewW, &previewH);
#endif
                    videoFormat = m_secCamera->getVideoFormat();
                    m_secCamera->getVideoSize(&videoW, &videoH);

                    m_getRatioSize(videoW, videoH,
                                   m_orgVideoRect.w, m_orgVideoRect.h,
                                   &cropX, &cropY,
                                   &cropW, &cropH,
                                   m_secCamera->getZoom());

                    ALOGV("DEBUG(%s):cropX = %d, cropY = %d, cropW = %d, cropH = %d",
                             __func__, cropX, cropY, cropW, cropH);

#ifdef USE_3DNR_DMAOUT
                    csc_set_src_format(m_exynosVideoCSC,
                                       videoW, videoH,
                                       cropX, cropY, cropW, cropH,
                                       V4L2_PIX_2_HAL_PIXEL_FORMAT(videoFormat),
                                       0);
#else
                    csc_set_src_format(m_exynosVideoCSC,
                                       previewW, previewH - 8,
                                       0, 0, previewW, previewH - 8,
                                       V4L2_PIX_2_HAL_PIXEL_FORMAT(previewFormat),
                                       0);
#endif

                    csc_set_dst_format(m_exynosVideoCSC,
                                       m_orgVideoRect.w, m_orgVideoRect.h,
                                       0, 0, m_orgVideoRect.w, m_orgVideoRect.h,
                                       V4L2_PIX_2_HAL_PIXEL_FORMAT(videoFormat),
                                       1);

#ifdef USE_3DNR_DMAOUT
                    csc_set_src_buffer(m_exynosVideoCSC,
                                      (unsigned char *)videoBuf.virt.extP[0],
                                      (unsigned char *)videoBuf.virt.extP[1],
                                      (unsigned char *)videoBuf.virt.extP[2],
                                       0);
#else
                    csc_set_src_buffer(m_exynosVideoCSC,
                                      (unsigned char *)copy_previewBuf.virt.extP[0],
                                      (unsigned char *)copy_previewBuf.virt.extP[2],
                                      (unsigned char *)copy_previewBuf.virt.extP[1],
                                       0);
#endif

                    ExynosBuffer dstBuf;
                    m_getAlignedYUVSize(videoFormat, m_orgVideoRect.w, m_orgVideoRect.h, &dstBuf);

#ifdef USE_3DNR_DMAOUT
                    dstBuf.virt.extP[0] = (char *)m_resizedVideoHeap[videoBuf.reserved.p]->data;
#else
                    dstBuf.virt.extP[0] = (char *)m_resizedVideoHeap[m_cntVideoBuf]->data;
#endif
                    for (int i = 1; i < 3; i++) {
                        if (dstBuf.size.extS[i] != 0)
                            dstBuf.virt.extP[i] = dstBuf.virt.extP[i-1] + dstBuf.size.extS[i-1];
                    }

                    csc_set_dst_buffer(m_exynosVideoCSC,
                                      (unsigned char *)dstBuf.virt.extP[0],
                                      (unsigned char *)dstBuf.virt.extP[1],
                                      (unsigned char *)dstBuf.virt.extP[2],
                                      0);

                    if (csc_convert(m_exynosVideoCSC) != 0)
                        ALOGE("ERR(%s):csc_convert() fail", __func__);
                } else {
                    ALOGE("ERR(%s):m_exynosVideoCSC == NULL", __func__);
                }
#ifdef USE_3DNR_DMAOUT
                m_dataCbTimestamp(timestamp, CAMERA_MSG_VIDEO_FRAME,
                                  m_resizedVideoHeap[videoBuf.reserved.p], 0, m_callbackCookie);
#else
                m_dataCbTimestamp(timestamp, CAMERA_MSG_VIDEO_FRAME,
                                  m_resizedVideoHeap[m_cntVideoBuf], 0, m_callbackCookie);
                m_cntVideoBuf++;
                if (m_cntVideoBuf == NUM_OF_VIDEO_BUF)
                    m_cntVideoBuf = 0;
#endif
            }

            // HACK : This must can handle on  releaseRecordingFrame()
#ifdef USE_3DNR_DMAOUT
            m_secCamera->putVideoBuf(&videoBuf);
#endif
            m_numOfAvailableVideoBuf++;
            if (NUM_OF_VIDEO_BUF <= m_numOfAvailableVideoBuf)
                m_numOfAvailableVideoBuf = NUM_OF_VIDEO_BUF;
            // until here
        } else
            usleep(1000); // sleep 1msec for stopRecording
    }

    return true;
}

bool ExynosCameraHWInterface::m_autoFocusThreadFunc(void)
{
    int count =0;
    bool afResult = false;
    ALOGV("DEBUG(%s):starting", __func__);

    /* block until we're told to start.  we don't want to use
     * a restartable thread and requestExitAndWait() in cancelAutoFocus()
     * because it would cause deadlock between our callbacks and the
     * caller of cancelAutoFocus() which both want to grab the same lock
     * in CameraServices layer.
     */
    m_focusLock.lock();
    /* check early exit request */
    if (m_exitAutoFocusThread == true) {
        m_focusLock.unlock();
        ALOGV("DEBUG(%s):exiting on request0", __func__);
        return true;
    }

    m_focusCondition.wait(m_focusLock);
    /* check early exit request */
    if (m_exitAutoFocusThread == true) {
        m_focusLock.unlock();
        ALOGV("DEBUG(%s):exiting on request1", __func__);
        return true;
    }
    m_focusLock.unlock();

    if (m_secCamera->autoFocus() == false) {
        ALOGE("ERR(%s):Fail on m_secCamera->autoFocus()", __func__);
        return false;
    }

    switch (m_secCamera->getFucusModeResult()) {
    case 0:
        ALOGV("DEBUG(%s):AF Cancelled !!", __func__);
        afResult = true;
        break;
    case 1:
        ALOGV("DEBUG(%s):AF Success!!", __func__);
        afResult = true;
        break;
    default:
        ALOGV("DEBUG(%s):AF Fail !!", __func__);
        afResult = false;
        break;
    }

    // CAMERA_MSG_FOCUS only takes a bool.  true for
    // finished and false for failure.  cancel is still
    // considered a true result.
    if (m_msgEnabled & CAMERA_MSG_FOCUS)
        m_notifyCb(CAMERA_MSG_FOCUS, afResult, 0, m_callbackCookie);

    ALOGV("DEBUG(%s):exiting with no error", __func__);
    return true;
}

bool ExynosCameraHWInterface::m_startPictureInternal(void)
{
    if (m_pictureRunning == true) {
        ALOGE("ERR(%s):Aready m_pictureRunning is running", __func__);
        return false;
    }

    int pictureW, pictureH, pictureFormat;
    unsigned int pictureFrameSize, pictureChromaSize;
    ExynosBuffer nullBuf;
    int numPlanes;

    m_secCamera->getPictureSize(&pictureW, &pictureH);
    pictureFormat = m_secCamera->getPictureFormat();
    PLANAR_FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(V4L2_PIX_FMT_NV16), pictureW, pictureH, &pictureFrameSize, 
					 &pictureChromaSize);
    numPlanes = NUM_PLANES(V4L2_PIX_2_HAL_PIXEL_FORMAT(V4L2_PIX_FMT_NV16));
#if 0
    if (m_rawHeap) {
        m_rawHeap->release(m_rawHeap);
        m_rawHeap = 0;
    }
    m_rawHeap = m_getMemoryCb(-1, pictureFramesize, 1, NULL);
    if (!m_rawHeap) {
        ALOGE("ERR(%s):m_getMemoryCb(m_rawHeap, size(%d) fail", __func__, pictureFramesize);
        return false;
    }

    pictureFramesize = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat), pictureW, pictureH);
#endif
    for (int i = 0; i < NUM_OF_PICTURE_BUF; i++) {
		for (int j = 0; j < 3; j++)
			if (m_pictureFds[i][j] >= 0) {
				close(m_pictureFds[i][j]);
				m_pictureFds[i][j] = -1;
			}

		m_pictureFds[i][0] = ion_alloc(m_ion_client, pictureFrameSize, 0, ION_HEAP_SYSTEM_MASK, 0);
		if (m_pictureFds[i][0] < 0) {
			ALOGE("ERR(%s):ion_alloc(m_pictureFds[%d], size(%d) fail", __func__, i, pictureFrameSize); 
			return false;
		}

		for (int j = 1; j < numPlanes; j++) {
			m_pictureFds[i][j] = ion_alloc(m_ion_client, pictureChromaSize, 0, ION_HEAP_SYSTEM_MASK, 0);
			if (m_pictureFds[i][j]) {
				ALOGE("ERR(%s):ion_alloc(m_pictureFds[%d][%d], size(%d) fail", __func__, i, j, pictureFrameSize); 
				return false;
			}
		}
		m_getAlignedYUVSize(pictureFormat, pictureW, pictureH, &m_pictureBuf);

		m_pictureBuf.fd.extFd[0] = m_pictureFds[i][0];
		for (int j = 1; j < 3; j++) {
			if (m_pictureBuf.size.extS[j] != 0)
				m_pictureBuf.fd.extFd[j] = m_pictureFds[i][j];
			else
				m_pictureBuf.fd.extFd[j] = -1;
		}

		m_pictureBuf.reserved.p = i;

		m_secCamera->setPictureBuf(&m_pictureBuf);
    }

    // zero shutter lag
    if (m_secCamera->startPicture() == false) {
        ALOGE("ERR(%s):Fail on m_secCamera->startPicture()", __func__);
        return false;
    }

    m_numOfAvaliblePictureBuf = 0;
    m_pictureBuf = nullBuf;

    for (int i = 0; i < NUM_OF_PICTURE_BUF; i++) {
        m_oldPictureBufQueue[i].buf = nullBuf;
        m_oldPictureBufQueue[i].next = NULL;
    }

    m_oldPictureBufQueueHead = &m_oldPictureBufQueue[0];

    m_pictureRunning = true;

    return true;

}

bool ExynosCameraHWInterface::m_stopPictureInternal(void)
{
    if (m_pictureRunning == false) {
        ALOGE("ERR(%s):Aready m_pictureRunning is stop", __func__);
        return false;
    }

    if (m_secCamera->flagStartPicture() == true
        && m_secCamera->stopPicture() == false)
        ALOGE("ERR(%s):Fail on m_secCamera->stopPicture()", __func__);

    for (int i = 0; i < NUM_OF_PICTURE_BUF; i++) {
        if (m_pictureHeap[i]) {
            m_pictureHeap[i]->release(m_pictureHeap[i]);
            m_pictureHeap[i] = 0;
        }
    }

    if (m_rawHeap) {
        m_rawHeap->release(m_rawHeap);
        m_rawHeap = 0;
    }

    m_pictureRunning = false;

    return true;
}

bool ExynosCameraHWInterface::m_pictureThreadFunc(void)
{
    bool ret = false;
    int pictureW, pictureH, pictureFramesize = 0;
    int pictureFormat;
    int cropX, cropY, cropW, cropH = 0;

    ExynosBuffer pictureBuf;
    ExynosBuffer jpegBuf;

    camera_memory_t *JpegHeap = NULL;
    camera_memory_t *JpegHeapOut = NULL;

    m_secCamera->getPictureSize(&pictureW, &pictureH);
    pictureFormat = m_secCamera->getPictureFormat();
    pictureFramesize = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat), pictureW, pictureH);

    JpegHeap = m_getMemoryCb(-1, pictureFramesize, 1, 0);
    if (!JpegHeap) {
        ALOGE("ERR(%s):m_getMemoryCb(JpegHeap, size(%d) fail", __func__, pictureFramesize);
        return false;
    }

    // resize from pictureBuf(max size) to rawHeap(user's set size)
    if (m_exynosPictureCSC) {
        m_getRatioSize(pictureW, pictureH,
                       m_orgPictureRect.w, m_orgPictureRect.h,
                       &cropX, &cropY,
                       &cropW, &cropH,
                       m_secCamera->getZoom());

        ALOGV("DEBUG(%s):cropX = %d, cropY = %d, cropW = %d, cropH = %d",
              __func__, cropX, cropY, cropW, cropH);

        csc_set_src_format(m_exynosPictureCSC,
                           pictureW, pictureH,
                           cropX, cropY, cropW, cropH,
                           V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat),
                           1);
                           //0);

        csc_set_dst_format(m_exynosPictureCSC,
                           m_orgPictureRect.w, m_orgPictureRect.h,
                           0, 0, m_orgPictureRect.w, m_orgPictureRect.h,
                           V4L2_PIX_2_HAL_PIXEL_FORMAT(V4L2_PIX_FMT_NV16),
                           1);
                           //0);

        csc_set_src_buffer(m_exynosPictureCSC,
                           (unsigned char *)m_pictureBuf.virt.extP[0],
                           (unsigned char *)m_pictureBuf.virt.extP[1],
                           (unsigned char *)m_pictureBuf.virt.extP[2],
                           0);

        pictureBuf.size.extS[0] = ALIGN(m_orgPictureRect.w, 16) * ALIGN(m_orgPictureRect.h, 16) * 2;
        pictureBuf.size.extS[1] = 0;
        pictureBuf.size.extS[2] = 0;

        pictureBuf.virt.extP[0] = (char *)m_rawHeap->data;

        csc_set_dst_buffer(m_exynosPictureCSC,
                           (unsigned char *)pictureBuf.virt.extP[0],
                           (unsigned char *)pictureBuf.virt.extP[1],
                           (unsigned char *)pictureBuf.virt.extP[2],
                           0);

        if (csc_convert(m_exynosPictureCSC) != 0)
            ALOGE("ERR(%s):csc_convert() fail", __func__);
    } else {
        ALOGE("ERR(%s):m_exynosPictureCSC == NULL", __func__);
    }

    if (m_msgEnabled & CAMERA_MSG_SHUTTER)
        m_notifyCb(CAMERA_MSG_SHUTTER, 0, 0, m_callbackCookie);

    m_getAlignedYUVSize(V4L2_PIX_FMT_NV16, m_orgPictureRect.w, m_orgPictureRect.h, &pictureBuf);

    for (int i = 1; i < 3; i++) {
        if (pictureBuf.size.extS[i] != 0)
            pictureBuf.virt.extP[i] = pictureBuf.virt.extP[i-1] + pictureBuf.size.extS[i-1];

        ALOGV("(%s): pictureBuf.size.extS[%d] = %d", __func__, i, pictureBuf.size.extS[i]);
    }

    if (m_msgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
        jpegBuf.virt.p = (char *)JpegHeap->data;
        jpegBuf.size.s = pictureFramesize;

        ExynosRect jpegRect;
        jpegRect.w = m_orgPictureRect.w;
        jpegRect.h = m_orgPictureRect.h;
        jpegRect.colorFormat = V4L2_PIX_FMT_NV16;

        if (m_secCamera->yuv2Jpeg(&pictureBuf, &jpegBuf, &jpegRect) == false) {
            ALOGE("ERR(%s):yuv2Jpeg() fail", __func__);
            m_stateLock.lock();
            m_captureInProgress = false;
            m_pictureLock.lock();
            m_pictureCondition.signal();
            m_pictureLock.unlock();
            m_stateLock.unlock();
            goto out;
        }
    }

    m_stateLock.lock();
    m_captureInProgress = false;
    m_pictureLock.lock();
    m_pictureCondition.signal();
    m_pictureLock.unlock();
    m_stateLock.unlock();

    if (m_msgEnabled & CAMERA_MSG_RAW_IMAGE)
        m_dataCb(CAMERA_MSG_RAW_IMAGE, m_rawHeap, 0, NULL, m_callbackCookie);

    /* TODO: Currently framework dose not support CAMERA_MSG_RAW_IMAGE_NOTIFY callback */
    /*
    if (m_msgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY)
        m_dataCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, m_rawHeap, 0, NULL, m_callbackCookie);
    */

    if (m_msgEnabled & CAMERA_MSG_POSTVIEW_FRAME)
        m_dataCb(CAMERA_MSG_POSTVIEW_FRAME, m_rawHeap, 0, NULL, m_callbackCookie);

    if (m_msgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
        JpegHeapOut = m_getMemoryCb(-1, jpegBuf.size.s, 1, 0);
        if (!JpegHeapOut) {
            ALOGE("ERR(%s):m_getMemoryCb(JpegHeapOut, size(%d) fail", __func__, jpegBuf.size.s);
            return false;
        }

        // TODO : we shall pass JpegHeap mem directly?
        memcpy(JpegHeapOut->data, JpegHeap->data, jpegBuf.size.s);

        m_dataCb(CAMERA_MSG_COMPRESSED_IMAGE, JpegHeapOut, 0, NULL, m_callbackCookie);
    }

    if (m_videoStart == false)
        stopPreview();

    ALOGV("DEBUG(%s):m_pictureThread end", __func__);

    ret = true;

out:

    if (JpegHeapOut) {
        JpegHeapOut->release(JpegHeapOut);
        JpegHeapOut = 0;
    }

    if (JpegHeap) {
        JpegHeap->release(JpegHeap);
        JpegHeap = 0;
    }

    return ret;
}

#ifdef LOG_NDEBUG
bool ExynosCameraHWInterface::m_fileDump(char *filename, void *srcBuf, uint32_t size)
{
    FILE *yuv_fd = NULL;
    char *buffer = NULL;
    static int count = 0;

    yuv_fd = fopen(filename, "w+");

    if (yuv_fd == NULL) {
        ALOGE("ERR file open fail: %s", filename);
        return 0;
    }

    buffer = (char *)malloc(size);

    if (buffer == NULL) {
        ALOGE("ERR malloc file");
        fclose(yuv_fd);
        return 0;
    }

    memcpy(buffer, srcBuf, size);

    fflush(stdout);

    fwrite(buffer, 1, size, yuv_fd);

    fflush(yuv_fd);

    if (yuv_fd)
        fclose(yuv_fd);
    if (buffer)
        free(buffer);

    ALOGV("filedump(%s) is successed!!", filename);
    return true;
}
#endif

void ExynosCameraHWInterface::m_setSkipFrame(int frame)
{
    Mutex::Autolock lock(m_skipFrameLock);
    if (frame < m_skipFrame)
        return;

    m_skipFrame = frame;
}

int ExynosCameraHWInterface::m_saveJpeg( unsigned char *real_jpeg, int jpeg_size)
{
    FILE *yuv_fp = NULL;
    char filename[100], *buffer = NULL;

    /* file create/open, note to "wb" */
    yuv_fp = fopen("/data/camera_dump.jpeg", "wb");
    if (yuv_fp == NULL) {
        ALOGE("Save jpeg file open error");
        return -1;
    }

    ALOGV("DEBUG(%s):[BestIQ]  real_jpeg size ========>  %d", __func__, jpeg_size);
    buffer = (char *) malloc(jpeg_size);
    if (buffer == NULL) {
        ALOGE("Save YUV] buffer alloc failed");
        if (yuv_fp)
            fclose(yuv_fp);

        return -1;
    }

    memcpy(buffer, real_jpeg, jpeg_size);

    fflush(stdout);

    fwrite(buffer, 1, jpeg_size, yuv_fp);

    fflush(yuv_fp);

    if (yuv_fp)
            fclose(yuv_fp);
    if (buffer)
            free(buffer);

    return 0;
}

void ExynosCameraHWInterface::m_savePostView(const char *fname, uint8_t *buf, uint32_t size)
{
    int nw;
    int cnt = 0;
    uint32_t written = 0;

    ALOGD("opening file [%s]", fname);
    int fd = open(fname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        ALOGE("failed to create file [%s]: %s", fname, strerror(errno));
        return;
    }

    ALOGD("writing %d bytes to file [%s]", size, fname);
    while (written < size) {
        nw = ::write(fd, buf + written, size - written);
        if (nw < 0) {
            ALOGE("failed to write to file %d [%s]: %s",written,fname, strerror(errno));
            break;
        }
        written += nw;
        cnt++;
    }
    ALOGD("done writing %d bytes to file [%s] in %d passes",size, fname, cnt);
    ::close(fd);
}

bool ExynosCameraHWInterface::m_scaleDownYuv422(char *srcBuf, uint32_t srcWidth, uint32_t srcHeight,
                                             char *dstBuf, uint32_t dstWidth, uint32_t dstHeight)
{
    int32_t step_x, step_y;
    int32_t iXsrc, iXdst;
    int32_t x, y, src_y_start_pos, dst_pos, src_pos;

    if (dstWidth % 2 != 0 || dstHeight % 2 != 0) {
        ALOGE("scale_down_yuv422: invalid width, height for scaling");
        return false;
    }

    step_x = srcWidth / dstWidth;
    step_y = srcHeight / dstHeight;

    dst_pos = 0;
    for (uint32_t y = 0; y < dstHeight; y++) {
        src_y_start_pos = (y * step_y * (srcWidth * 2));

        for (uint32_t x = 0; x < dstWidth; x += 2) {
            src_pos = src_y_start_pos + (x * (step_x * 2));

            dstBuf[dst_pos++] = srcBuf[src_pos    ];
            dstBuf[dst_pos++] = srcBuf[src_pos + 1];
            dstBuf[dst_pos++] = srcBuf[src_pos + 2];
            dstBuf[dst_pos++] = srcBuf[src_pos + 3];
        }
    }

    return true;
}

bool ExynosCameraHWInterface::m_YUY2toNV21(void *srcBuf, void *dstBuf, uint32_t srcWidth, uint32_t srcHeight)
{
    int32_t        x, y, src_y_start_pos, dst_cbcr_pos, dst_pos, src_pos;
    unsigned char *srcBufPointer = (unsigned char *)srcBuf;
    unsigned char *dstBufPointer = (unsigned char *)dstBuf;

    dst_pos = 0;
    dst_cbcr_pos = srcWidth*srcHeight;
    for (uint32_t y = 0; y < srcHeight; y++) {
        src_y_start_pos = (y * (srcWidth * 2));

        for (uint32_t x = 0; x < (srcWidth * 2); x += 2) {
            src_pos = src_y_start_pos + x;

            dstBufPointer[dst_pos++] = srcBufPointer[src_pos];
        }
    }
    for (uint32_t y = 0; y < srcHeight; y += 2) {
        src_y_start_pos = (y * (srcWidth * 2));

        for (uint32_t x = 0; x < (srcWidth * 2); x += 4) {
            src_pos = src_y_start_pos + x;

            dstBufPointer[dst_cbcr_pos++] = srcBufPointer[src_pos + 3];
            dstBufPointer[dst_cbcr_pos++] = srcBufPointer[src_pos + 1];
        }
    }

    return true;
}

bool ExynosCameraHWInterface::m_checkVideoStartMarker(unsigned char *pBuf)
{
    if (!pBuf) {
        ALOGE("m_checkVideoStartMarker() => pBuf is NULL");
        return false;
    }

    if (HIBYTE(VIDEO_COMMENT_MARKER_H) == * pBuf      && LOBYTE(VIDEO_COMMENT_MARKER_H) == *(pBuf + 1) &&
        HIBYTE(VIDEO_COMMENT_MARKER_L) == *(pBuf + 2) && LOBYTE(VIDEO_COMMENT_MARKER_L) == *(pBuf + 3))
        return true;

    return false;
}

bool ExynosCameraHWInterface::m_checkEOIMarker(unsigned char *pBuf)
{
    if (!pBuf) {
        ALOGE("m_checkEOIMarker() => pBuf is NULL");
        return false;
    }

    // EOI marker [FF D9]
    if (HIBYTE(JPEG_EOI_MARKER) == *pBuf && LOBYTE(JPEG_EOI_MARKER) == *(pBuf + 1))
        return true;

    return false;
}

bool ExynosCameraHWInterface::m_findEOIMarkerInJPEG(unsigned char *pBuf, int dwBufSize, int *pnJPEGsize)
{
    if (NULL == pBuf || 0 >= dwBufSize) {
        ALOGE("m_findEOIMarkerInJPEG() => There is no contents.");
        return false;
    }

    unsigned char *pBufEnd = pBuf + dwBufSize;

    while (pBuf < pBufEnd) {
        if (m_checkEOIMarker(pBuf++))
            return true;

        (*pnJPEGsize)++;
    }

    return false;
}

bool ExynosCameraHWInterface::m_splitFrame(unsigned char *pFrame, int dwSize,
                    int dwJPEGLineLength, int dwVideoLineLength, int dwVideoHeight,
                    void *pJPEG, int *pdwJPEGSize,
                    void *pVideo, int *pdwVideoSize)
{
    ALOGV("DEBUG(%s):===========m_splitFrame Start==============", __func__);

    if (NULL == pFrame || 0 >= dwSize) {
        ALOGE("There is no contents (pFrame=%p, dwSize=%d", pFrame, dwSize);
        return false;
    }

    if (0 == dwJPEGLineLength || 0 == dwVideoLineLength) {
        ALOGE("There in no input information for decoding interleaved jpeg");
        return false;
    }

    unsigned char *pSrc = pFrame;
    unsigned char *pSrcEnd = pFrame + dwSize;

    unsigned char *pJ = (unsigned char *)pJPEG;
    int dwJSize = 0;
    unsigned char *pV = (unsigned char *)pVideo;
    int dwVSize = 0;

    bool bRet = false;
    bool isFinishJpeg = false;

    while (pSrc < pSrcEnd) {
        // Check video start marker
        if (m_checkVideoStartMarker(pSrc)) {
            int copyLength;

            if (pSrc + dwVideoLineLength <= pSrcEnd)
                copyLength = dwVideoLineLength;
            else
                copyLength = pSrcEnd - pSrc - VIDEO_COMMENT_MARKER_LENGTH;

            // Copy video data
            if (pV) {
                memcpy(pV, pSrc + VIDEO_COMMENT_MARKER_LENGTH, copyLength);
                pV += copyLength;
                dwVSize += copyLength;
            }

            pSrc += copyLength + VIDEO_COMMENT_MARKER_LENGTH;
        } else {
            // Copy pure JPEG data
            int size = 0;
            int dwCopyBufLen = dwJPEGLineLength <= pSrcEnd-pSrc ? dwJPEGLineLength : pSrcEnd - pSrc;

            if (m_findEOIMarkerInJPEG((unsigned char *)pSrc, dwCopyBufLen, &size)) {
                isFinishJpeg = true;
                size += 2;  // to count EOF marker size
            } else {
                if ((dwCopyBufLen == 1) && (pJPEG < pJ)) {
                    unsigned char checkBuf[2] = { *(pJ - 1), *pSrc };

                    if (m_checkEOIMarker(checkBuf))
                        isFinishJpeg = true;
                }
                size = dwCopyBufLen;
            }

            memcpy(pJ, pSrc, size);

            dwJSize += size;

            pJ += dwCopyBufLen;
            pSrc += dwCopyBufLen;
        }
        if (isFinishJpeg)
            break;
    }

    if (isFinishJpeg) {
        bRet = true;
        if (pdwJPEGSize)
            *pdwJPEGSize = dwJSize;
        if (pdwVideoSize)
            *pdwVideoSize = dwVSize;
    } else {
        ALOGE("DecodeInterleaveJPEG_WithOutDT() => Can not find EOI");
        bRet = false;
        if (pdwJPEGSize)
            *pdwJPEGSize = 0;
        if (pdwVideoSize)
            *pdwVideoSize = 0;
    }
    ALOGV("DEBUG(%s):===========m_splitFrame end==============", __func__);

    return bRet;
}

int ExynosCameraHWInterface::m_decodeInterleaveData(unsigned char *pInterleaveData,
                                                 int interleaveDataSize,
                                                 int yuvWidth,
                                                 int yuvHeight,
                                                 int *pJpegSize,
                                                 void *pJpegData,
                                                 void *pYuvData)
{
    if (pInterleaveData == NULL)
        return false;

    bool ret = true;
    unsigned int *interleave_ptr = (unsigned int *)pInterleaveData;
    unsigned char *jpeg_ptr = (unsigned char *)pJpegData;
    unsigned char *yuv_ptr = (unsigned char *)pYuvData;
    unsigned char *p;
    int jpeg_size = 0;
    int yuv_size = 0;

    int i = 0;

    ALOGV("DEBUG(%s):m_decodeInterleaveData Start~~~", __func__);
    while (i < interleaveDataSize) {
        if ((*interleave_ptr == 0xFFFFFFFF) || (*interleave_ptr == 0x02FFFFFF) ||
                (*interleave_ptr == 0xFF02FFFF)) {
            // Padding Data
            interleave_ptr++;
            i += 4;
        } else if ((*interleave_ptr & 0xFFFF) == 0x05FF) {
            // Start-code of YUV Data
            p = (unsigned char *)interleave_ptr;
            p += 2;
            i += 2;

            // Extract YUV Data
            if (pYuvData != NULL) {
                memcpy(yuv_ptr, p, yuvWidth * 2);
                yuv_ptr += yuvWidth * 2;
                yuv_size += yuvWidth * 2;
            }
            p += yuvWidth * 2;
            i += yuvWidth * 2;

            // Check End-code of YUV Data
            if ((*p == 0xFF) && (*(p + 1) == 0x06)) {
                interleave_ptr = (unsigned int *)(p + 2);
                i += 2;
            } else {
                ret = false;
                break;
            }
        } else {
            // Extract JPEG Data
            if (pJpegData != NULL) {
                memcpy(jpeg_ptr, interleave_ptr, 4);
                jpeg_ptr += 4;
                jpeg_size += 4;
            }
            interleave_ptr++;
            i += 4;
        }
    }
    if (ret) {
        if (pJpegData != NULL) {
            // Remove Padding after EOI
            for (i = 0; i < 3; i++) {
                if (*(--jpeg_ptr) != 0xFF) {
                    break;
                }
                jpeg_size--;
            }
            *pJpegSize = jpeg_size;

        }
        // Check YUV Data Size
        if (pYuvData != NULL) {
            if (yuv_size != (yuvWidth * yuvHeight * 2)) {
                ret = false;
            }
        }
    }
    ALOGV("DEBUG(%s):m_decodeInterleaveData End~~~", __func__);
    return ret;
}

bool ExynosCameraHWInterface::m_isSupportedPreviewSize(const int width,
                                               const int height) const
{
    unsigned int i;

    for (i = 0; i < m_supportedPreviewSizes.size(); i++) {
        if (m_supportedPreviewSizes[i].width == width &&
                m_supportedPreviewSizes[i].height == height)
            return true;
    }

    return false;
}

void ExynosCameraHWInterface::m_getAlignedYUVSize(int colorFormat, int w, int h, ExynosBuffer *buf)
{
    switch (colorFormat) {
    // 1p
    case V4L2_PIX_FMT_RGB565 :
    case V4L2_PIX_FMT_YUYV :
    case V4L2_PIX_FMT_UYVY :
    case V4L2_PIX_FMT_VYUY :
    case V4L2_PIX_FMT_YVYU :
        buf->size.extS[0] = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(colorFormat), w, h);
        buf->size.extS[1] = 0;
        buf->size.extS[2] = 0;
        break;
    // 2p
    case V4L2_PIX_FMT_NV12 :
    case V4L2_PIX_FMT_NV12T :
    case V4L2_PIX_FMT_NV21 :
        buf->size.extS[0] = ALIGN(w,   16) * ALIGN(h,   16);
        buf->size.extS[1] = ALIGN(w/2, 16) * ALIGN(h/2, 16);
        buf->size.extS[2] = 0;
        break;
    case V4L2_PIX_FMT_NV12M :
    case V4L2_PIX_FMT_NV12MT_16X16 :
        buf->size.extS[0] = ALIGN(ALIGN(w, 16) * ALIGN(h,     16), 2048);
        buf->size.extS[1] = ALIGN(ALIGN(w, 16) * ALIGN(h >> 1, 8), 2048);
        buf->size.extS[2] = 0;
        break;
    case V4L2_PIX_FMT_NV16 :
    case V4L2_PIX_FMT_NV61 :
        buf->size.extS[0] = ALIGN(w, 16) * ALIGN(h, 16);
        buf->size.extS[1] = ALIGN(w, 16) * ALIGN(h,  16);
        buf->size.extS[2] = 0;
        break;
     // 3p
    case V4L2_PIX_FMT_YUV420 :
    case V4L2_PIX_FMT_YVU420 :
        buf->size.extS[0] = (w * h);
        buf->size.extS[1] = (w * h) >> 2;
        buf->size.extS[2] = (w * h) >> 2;
        break;
    case V4L2_PIX_FMT_YUV420M:
    case V4L2_PIX_FMT_YVU420M :
    case V4L2_PIX_FMT_YUV422P :
        buf->size.extS[0] = ALIGN(w,  16) * ALIGN(h,  16);
        buf->size.extS[1] = ALIGN(w/2, 8) * ALIGN(h/2, 8);
        buf->size.extS[2] = ALIGN(w/2, 8) * ALIGN(h/2, 8);
        break;
    default:
        ALOGE("ERR(%s):unmatched colorFormat(%d)", __func__, colorFormat);
        return;
        break;
    }
}

bool ExynosCameraHWInterface::m_getResolutionList(String8 & string8Buf, char * strBuf, int w, int h)
{
    bool ret = false;
    bool flagFirst = true;

    // this is up to /packages/apps/Camera/res/values/arrays.xml
    int RESOLUTION_LIST[][2] =
    {
        { 3264, 2448},
        { 2592, 1936},
        { 2576, 1948},
        { 2560, 1920},
        { 2048, 1536},
        { 1920, 1080},
        { 1600, 1200},
        { 1280,  720},
        { 1024,  768},
        {  800,  600},
        {  800,  480},
        {  720,  480},
        {  640,  480},
        {  528,  432},
        {  480,  320},
        {  352,  288},
        {  320,  240},
        {  176,  144}
    };

    int sizeOfResSize = sizeof(RESOLUTION_LIST) / (sizeof(int) * 2);

    for (int i = 0; i < sizeOfResSize; i++) {
        if (   RESOLUTION_LIST[i][0] <= w
            && RESOLUTION_LIST[i][1] <= h) {
            if (flagFirst == true)
                flagFirst = false;
            else
                string8Buf.append(",");

            sprintf(strBuf, "%dx%d", RESOLUTION_LIST[i][0], RESOLUTION_LIST[i][1]);
            string8Buf.append(strBuf);

            ret = true;
        }
    }

    if (ret == false)
        ALOGE("ERR(%s):cannot find resolutions", __func__);

    return ret;
}

bool ExynosCameraHWInterface::m_getZoomRatioList(String8 & string8Buf, char * strBuf, int maxZoom, int start, int end)
{
    bool flagFirst = true;

    int cur = start;
    int step = (end - start) / maxZoom;

    for (int i = 0; i < maxZoom; i++) {
        sprintf(strBuf, "%d", cur);
        string8Buf.append(strBuf);
        string8Buf.append(",");
        cur += step;
    }

    sprintf(strBuf, "%d", end);
    string8Buf.append(strBuf);

    // ex : "100,130,160,190,220,250,280,310,340,360,400"

    return true;
}

int ExynosCameraHWInterface::m_bracketsStr2Ints(char *str, int num, ExynosRect2 *rect2s, int *weights)
{
    char *curStr = str;
    char buf[128];
    char *bracketsOpen;
    char *bracketsClose;

    int tempArray[5];
    int validFocusedAreas = 0;

    for (int i = 0; i < num; i++) {
        if (curStr == NULL)
            break;

        bracketsOpen = strchr(curStr, '(');
        if (bracketsOpen == NULL)
            break;

        bracketsClose = strchr(bracketsOpen, ')');
        if (bracketsClose == NULL)
            break;

        strncpy(buf, bracketsOpen, bracketsClose - bracketsOpen + 1);
        buf[bracketsClose - bracketsOpen + 1] = 0;

        if (m_subBracketsStr2Ints(5, buf, tempArray) == false) {
            ALOGE("ERR(%s):m_subBracketsStr2Ints(%s) fail", __func__, buf);
            break;
        }

        rect2s[i].x1 = tempArray[0];
        rect2s[i].y1 = tempArray[1];
        rect2s[i].x2 = tempArray[2];
        rect2s[i].y2 = tempArray[3];
        weights[i] = tempArray[4];

        validFocusedAreas++;

        curStr = bracketsClose;
    }
    return validFocusedAreas;
}

bool ExynosCameraHWInterface::m_subBracketsStr2Ints(int num, char *str, int *arr)
{
    if (str == NULL || arr == NULL) {
        ALOGE("ERR(%s):str or arr is NULL", __func__);
        return false;
    }

    // ex : (-10,-10,0,0,300)
    char buf[128];
    char *bracketsOpen;
    char *bracketsClose;
    char *tok;

    bracketsOpen = strchr(str, '(');
    if (bracketsOpen == NULL) {
        ALOGE("ERR(%s):no '('", __func__);
        return false;
    }

    bracketsClose = strchr(bracketsOpen, ')');
    if (bracketsClose == NULL) {
        ALOGE("ERR(%s):no ')'", __func__);
        return false;
    }

    strncpy(buf, bracketsOpen + 1, bracketsClose - bracketsOpen + 1);
    buf[bracketsClose - bracketsOpen + 1] = 0;

    tok = strtok(buf, ",");
    if (tok == NULL) {
        ALOGE("ERR(%s):strtok(%s) fail", __func__, buf);
        return false;
    }

    arr[0] = atoi(tok);

    for (int i = 1; i < num; i++) {
        tok = strtok(NULL, ",");
        if (tok == NULL) {
            if (i < num - 1) {
                ALOGE("ERR(%s):strtok() (index : %d, num : %d) fail", __func__, i, num);
                return false;
            }
            break;
        }

        arr[i] = atoi(tok);
    }

    return true;
}

bool ExynosCameraHWInterface::m_getRatioSize(int  src_w,  int   src_h,
                                             int  dst_w,  int   dst_h,
                                             int *crop_x, int *crop_y,
                                             int *crop_w, int *crop_h,
                                             int zoom)
{
    *crop_w = src_w;
    *crop_h = src_h;

    if (   src_w != dst_w
        || src_h != dst_h) {
        float src_ratio = 1.0f;
        float dst_ratio = 1.0f;

        // ex : 1024 / 768
        src_ratio = (float)src_w / (float)src_h;

        // ex : 352  / 288
        dst_ratio = (float)dst_w / (float)dst_h;

        if (src_ratio != dst_ratio) {
            if (dst_w * dst_h < src_w * src_h) {
                if (src_ratio <= dst_ratio) {
                    // shrink h
                    *crop_w = src_w;
                    *crop_h = src_w / dst_ratio;
                } else {
                    // shrink w
                    *crop_w = dst_h * dst_ratio;
                    *crop_h = dst_h;
                }
            } else {
                if (src_ratio <= dst_ratio) {
                    // shrink h
                    *crop_w = src_w;
                    *crop_h = src_w / dst_ratio;
                } else {
                    // shrink w
                    *crop_w = src_h * dst_ratio;
                    *crop_h = src_h;
                }
            }

            if (zoom != 0) {
                int zoomLevel = ((float)zoom + 10.0) / 10.0;
                *crop_w = (int)((float)*crop_w / zoomLevel);
                *crop_h = (int)((float)*crop_h / zoomLevel);
            }
        }
    }

    #define CAMERA_CROP_WIDTH_RESTRAIN_NUM  (0x2)
    unsigned int w_align = (*crop_w & (CAMERA_CROP_WIDTH_RESTRAIN_NUM - 1));
    if (w_align != 0) {
        if (  (CAMERA_CROP_WIDTH_RESTRAIN_NUM >> 1) <= w_align
            && *crop_w + (CAMERA_CROP_WIDTH_RESTRAIN_NUM - w_align) <= dst_w) {
            *crop_w += (CAMERA_CROP_WIDTH_RESTRAIN_NUM - w_align);
        }
        else
            *crop_w -= w_align;
    }

    #define CAMERA_CROP_HEIGHT_RESTRAIN_NUM  (0x2)
    unsigned int h_align = (*crop_h & (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - 1));
    if (h_align != 0) {
        if (  (CAMERA_CROP_HEIGHT_RESTRAIN_NUM >> 1) <= h_align
            && *crop_h + (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - h_align) <= dst_h) {
            *crop_h += (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - h_align);
        }
        else
            *crop_h -= h_align;
    }

    *crop_x = (src_w - *crop_w) >> 1;
    *crop_y = (src_h - *crop_h) >> 1;

    if (*crop_x & (CAMERA_CROP_WIDTH_RESTRAIN_NUM >> 1))
        *crop_x -= 1;

    if (*crop_y & (CAMERA_CROP_HEIGHT_RESTRAIN_NUM >> 1))
        *crop_y -= 1;

    return true;
}

int ExynosCameraHWInterface::m_calibratePosition(int w, int new_w, int pos)
{
    return (float)(pos * new_w) / (float)w;
}

static CameraInfo sCameraInfo[] = {
    {
        CAMERA_FACING_BACK,
        0,  /* orientation */
    },
    {
        CAMERA_FACING_FRONT,
        0,  /* orientation */
    }
};

/** Close this device */

static camera_device_t *g_cam_device;

static int HAL_camera_device_close(struct hw_device_t* device)
{
    ALOGV("DEBUG(%s):", __func__);
    if (device) {
        camera_device_t *cam_device = (camera_device_t *)device;
        delete static_cast<ExynosCameraHWInterface *>(cam_device->priv);
        free(cam_device);
        g_cam_device = 0;
    }
    return 0;
}

static inline ExynosCameraHWInterface *obj(struct camera_device *dev)
{
    return reinterpret_cast<ExynosCameraHWInterface *>(dev->priv);
}

/** Set the preview_stream_ops to which preview frames are sent */
static int HAL_camera_device_set_preview_window(struct camera_device *dev,
                                                struct preview_stream_ops *buf)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->setPreviewWindow(buf);
}

/** Set the notification and data callbacks */
static void HAL_camera_device_set_callbacks(struct camera_device *dev,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void* user)
{
    ALOGV("DEBUG(%s):", __func__);
    obj(dev)->setCallbacks(notify_cb, data_cb, data_cb_timestamp,
                           get_memory,
                           user);
}

/**
 * The following three functions all take a msg_type, which is a bitmask of
 * the messages defined in include/ui/Camera.h
 */

/**
 * Enable a message, or set of messages.
 */
static void HAL_camera_device_enable_msg_type(struct camera_device *dev, int32_t msg_type)
{
    ALOGV("DEBUG(%s):", __func__);
    obj(dev)->enableMsgType(msg_type);
}

/**
 * Disable a message, or a set of messages.
 *
 * Once received a call to disableMsgType(CAMERA_MSG_VIDEO_FRAME), camera
 * HAL should not rely on its client to call releaseRecordingFrame() to
 * release video recording frames sent out by the cameral HAL before and
 * after the disableMsgType(CAMERA_MSG_VIDEO_FRAME) call. Camera HAL
 * clients must not modify/access any video recording frame after calling
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME).
 */
static void HAL_camera_device_disable_msg_type(struct camera_device *dev, int32_t msg_type)
{
    ALOGV("DEBUG(%s):", __func__);
    obj(dev)->disableMsgType(msg_type);
}

/**
 * Query whether a message, or a set of messages, is enabled.  Note that
 * this is operates as an AND, if any of the messages queried are off, this
 * will return false.
 */
static int HAL_camera_device_msg_type_enabled(struct camera_device *dev, int32_t msg_type)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->msgTypeEnabled(msg_type);
}

/**
 * Start preview mode.
 */
static int HAL_camera_device_start_preview(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->startPreview();
}

/**
 * Stop a previously started preview.
 */
static void HAL_camera_device_stop_preview(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    obj(dev)->stopPreview();
}

/**
 * Returns true if preview is enabled.
 */
static int HAL_camera_device_preview_enabled(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->previewEnabled();
}

/**
 * Request the camera HAL to store meta data or real YUV data in the video
 * buffers sent out via CAMERA_MSG_VIDEO_FRAME for a recording session. If
 * it is not called, the default camera HAL behavior is to store real YUV
 * data in the video buffers.
 *
 * This method should be called before startRecording() in order to be
 * effective.
 *
 * If meta data is stored in the video buffers, it is up to the receiver of
 * the video buffers to interpret the contents and to find the actual frame
 * data with the help of the meta data in the buffer. How this is done is
 * outside of the scope of this method.
 *
 * Some camera HALs may not support storing meta data in the video buffers,
 * but all camera HALs should support storing real YUV data in the video
 * buffers. If the camera HAL does not support storing the meta data in the
 * video buffers when it is requested to do do, INVALID_OPERATION must be
 * returned. It is very useful for the camera HAL to pass meta data rather
 * than the actual frame data directly to the video encoder, since the
 * amount of the uncompressed frame data can be very large if video size is
 * large.
 *
 * @param enable if true to instruct the camera HAL to store
 *      meta data in the video buffers; false to instruct
 *      the camera HAL to store real YUV data in the video
 *      buffers.
 *
 * @return OK on success.
 */
static int HAL_camera_device_store_meta_data_in_buffers(struct camera_device *dev, int enable)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->storeMetaDataInBuffers(enable);
}

/**
 * Start record mode. When a record image is available, a
 * CAMERA_MSG_VIDEO_FRAME message is sent with the corresponding
 * frame. Every record frame must be released by a camera HAL client via
 * releaseRecordingFrame() before the client calls
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME). After the client calls
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME), it is the camera HAL's
 * responsibility to manage the life-cycle of the video recording frames,
 * and the client must not modify/access any video recording frames.
 */
static int HAL_camera_device_start_recording(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->startRecording();
}

/**
 * Stop a previously started recording.
 */
static void HAL_camera_device_stop_recording(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    obj(dev)->stopRecording();
}

/**
 * Returns true if recording is enabled.
 */
static int HAL_camera_device_recording_enabled(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->recordingEnabled();
}

/**
 * Release a record frame previously returned by CAMERA_MSG_VIDEO_FRAME.
 *
 * It is camera HAL client's responsibility to release video recording
 * frames sent out by the camera HAL before the camera HAL receives a call
 * to disableMsgType(CAMERA_MSG_VIDEO_FRAME). After it receives the call to
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME), it is the camera HAL's
 * responsibility to manage the life-cycle of the video recording frames.
 */
static void HAL_camera_device_release_recording_frame(struct camera_device *dev,
                                const void *opaque)
{
    ALOGV("DEBUG(%s):", __func__);
    obj(dev)->releaseRecordingFrame(opaque);
}

/**
 * Start auto focus, the notification callback routine is called with
 * CAMERA_MSG_FOCUS once when focusing is complete. autoFocus() will be
 * called again if another auto focus is needed.
 */
static int HAL_camera_device_auto_focus(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->autoFocus();
}

/**
 * Cancels auto-focus function. If the auto-focus is still in progress,
 * this function will cancel it. Whether the auto-focus is in progress or
 * not, this function will return the focus position to the default.  If
 * the camera does not support auto-focus, this is a no-op.
 */
static int HAL_camera_device_cancel_auto_focus(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->cancelAutoFocus();
}

/**
 * Take a picture.
 */
static int HAL_camera_device_take_picture(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->takePicture();
}

/**
 * Cancel a picture that was started with takePicture. Calling this method
 * when no picture is being taken is a no-op.
 */
static int HAL_camera_device_cancel_picture(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->cancelPicture();
}

/**
 * Set the camera parameters. This returns BAD_VALUE if any parameter is
 * invalid or not supported.
 */
static int HAL_camera_device_set_parameters(struct camera_device *dev,
                                            const char *parms)
{
    ALOGV("DEBUG(%s):", __func__);
    String8 str(parms);
    CameraParameters p(str);
    return obj(dev)->setParameters(p);
}

/** Return the camera parameters. */
char *HAL_camera_device_get_parameters(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    String8 str;
    CameraParameters parms = obj(dev)->getParameters();
    str = parms.flatten();
    return strdup(str.string());
}

static void HAL_camera_device_put_parameters(struct camera_device *dev, char *parms)
{
    ALOGV("DEBUG(%s):", __func__);
    free(parms);
}

/**
 * Send command to camera driver.
 */
static int HAL_camera_device_send_command(struct camera_device *dev,
                    int32_t cmd, int32_t arg1, int32_t arg2)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->sendCommand(cmd, arg1, arg2);
}

/**
 * Release the hardware resources owned by this object.  Note that this is
 * *not* done in the destructor.
 */
static void HAL_camera_device_release(struct camera_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    obj(dev)->release();
}

/**
 * Dump state of the camera hardware
 */
static int HAL_camera_device_dump(struct camera_device *dev, int fd)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->dump(fd);
}

static int HAL_getNumberOfCameras()
{
    ALOGV("DEBUG(%s):", __func__);
    return sizeof(sCameraInfo) / sizeof(sCameraInfo[0]);
}

static int HAL_getCameraInfo(int cameraId, struct camera_info *cameraInfo)
{
    ALOGV("DEBUG(%s):", __func__);
    memcpy(cameraInfo, &sCameraInfo[cameraId], sizeof(CameraInfo));
    return 0;
}

#define SET_METHOD(m) m : HAL_camera_device_##m

static camera_device_ops_t camera_device_ops = {
        SET_METHOD(set_preview_window),
        SET_METHOD(set_callbacks),
        SET_METHOD(enable_msg_type),
        SET_METHOD(disable_msg_type),
        SET_METHOD(msg_type_enabled),
        SET_METHOD(start_preview),
        SET_METHOD(stop_preview),
        SET_METHOD(preview_enabled),
        SET_METHOD(store_meta_data_in_buffers),
        SET_METHOD(start_recording),
        SET_METHOD(stop_recording),
        SET_METHOD(recording_enabled),
        SET_METHOD(release_recording_frame),
        SET_METHOD(auto_focus),
        SET_METHOD(cancel_auto_focus),
        SET_METHOD(take_picture),
        SET_METHOD(cancel_picture),
        SET_METHOD(set_parameters),
        SET_METHOD(get_parameters),
        SET_METHOD(put_parameters),
        SET_METHOD(send_command),
        SET_METHOD(release),
        SET_METHOD(dump),
};

#undef SET_METHOD

static int HAL_camera_device_open(const struct hw_module_t* module,
                                  const char *id,
                                  struct hw_device_t** device)
{
    ALOGV("DEBUG(%s):", __func__);

    int cameraId = atoi(id);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        ALOGE("ERR(%s):Invalid camera ID %s", __func__, id);
        return -EINVAL;
    }

    if (g_cam_device) {
        if (obj(g_cam_device)->getCameraId() == cameraId) {
            ALOGV("DEBUG(%s):returning existing camera ID %s", __func__, id);
            goto done;
        } else {
            ALOGE("ERR(%s):Cannot open camera %d. camera %d is already running!",
                    __func__, cameraId, obj(g_cam_device)->getCameraId());
            return -ENOSYS;
        }
    }

    g_cam_device = (camera_device_t *)malloc(sizeof(camera_device_t));
    if (!g_cam_device)
        return -ENOMEM;

    g_cam_device->common.tag     = HARDWARE_DEVICE_TAG;
    g_cam_device->common.version = 1;
    g_cam_device->common.module  = const_cast<hw_module_t *>(module);
    g_cam_device->common.close   = HAL_camera_device_close;

    g_cam_device->ops = &camera_device_ops;

    ALOGV("DEBUG(%s):open camera %s", __func__, id);

    g_cam_device->priv = new ExynosCameraHWInterface(cameraId, g_cam_device);

done:
    *device = (hw_device_t *)g_cam_device;
    ALOGV("DEBUG(%s):opened camera %s (%p)", __func__, id, *device);
    return 0;
}

static hw_module_methods_t camera_module_methods = {
            open : HAL_camera_device_open
};

extern "C" {
    struct camera_module HAL_MODULE_INFO_SYM = {
      common : {
          tag           : HARDWARE_MODULE_TAG,
          version_major : 1,
          version_minor : 0,
          id            : CAMERA_HARDWARE_MODULE_ID,
          name          : "orion camera HAL",
          author        : "Samsung Corporation",
          methods       : &camera_module_methods,
      },
      get_number_of_cameras : HAL_getNumberOfCameras,
      get_camera_info       : HAL_getCameraInfo
    };
}

}; // namespace android
