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
 * \brief     hearder file for Android Camera HAL
 * \author    thun.hwang(thun.hwang@samsung.com)
 * \date      2010/06/03
 *
 * <b>Revision History: </b>
 * - 2011/12/31 : thun.hwang(thun.hwang@samsung.com) \n
 *   Initial version
 *
 * - 2012/03/14 : sangwoo.park(sw5771.park@samsung.com) \n
 *   Change file, class name to ExynosXXX.
 *
 */

#ifndef EXYNOS_CAMERA_HW_INTERFACE_H

#include <utils/threads.h>
#include <utils/RefBase.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <hardware/camera.h>
#include <hardware/gralloc.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <media/hardware/MetadataBufferType.h>

#include "gralloc_priv.h"

#include "exynos_format.h"
#include "csc.h"
#include "ExynosCamera.h"

#include <fcntl.h>
#include <sys/mman.h>

#define USE_EGL                         (1)

#define  NUM_OF_PREVIEW_BUF             (8)
#define  NUM_OF_VIDEO_BUF               (8)
#define  NUM_OF_PICTURE_BUF             (6)
#define  NUM_OF_WAITING_PUT_PICTURE_BUF (1)

#define  NUM_OF_DETECTED_FACES          (32)

namespace android {

class ExynosCameraHWInterface : public virtual RefBase {
public:
    ExynosCameraHWInterface(int cameraId, camera_device_t *dev);
    virtual             ~ExynosCameraHWInterface();

    virtual status_t    setPreviewWindow(preview_stream_ops *w);
    virtual void        setCallbacks(camera_notify_callback notify_cb,
                                     camera_data_callback data_cb,
                                     camera_data_timestamp_callback data_cb_timestamp,
                                     camera_request_memory get_memory,
                                     void *user);

    virtual void        enableMsgType(int32_t msgType);
    virtual void        disableMsgType(int32_t msgType);
    virtual bool        msgTypeEnabled(int32_t msgType);

    virtual status_t    startPreview();
    virtual void        stopPreview();
    virtual bool        previewEnabled();

    virtual status_t    storeMetaDataInBuffers(bool enable);

    virtual status_t    startRecording();
    virtual void        stopRecording();
    virtual bool        recordingEnabled();
    virtual void        releaseRecordingFrame(const void *opaque);

    virtual status_t    autoFocus();
    virtual status_t    cancelAutoFocus();

    virtual status_t    takePicture();
    virtual status_t    cancelPicture();

    virtual status_t    setParameters(const CameraParameters& params);
    virtual CameraParameters  getParameters() const;
    virtual status_t    sendCommand(int32_t command, int32_t arg1, int32_t arg2);

    virtual void        release();

    virtual status_t    dump(int fd) const;

    inline  int         getCameraId() const;

private:
    class PreviewThread : public Thread {
        ExynosCameraHWInterface *mHardware;
    public:
        PreviewThread(ExynosCameraHWInterface *hw):
        Thread(false),
        mHardware(hw) { }
        virtual void onFirstRef() {
            //run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
            run("CameraPreviewThread", PRIORITY_DEFAULT);
        }
        virtual bool threadLoop() {
            mHardware->m_previewThreadFuncWrapper();
            return false;
        }
    };

    class VideoThread : public Thread {
        ExynosCameraHWInterface *mHardware;
    public:
        VideoThread(ExynosCameraHWInterface *hw):
        Thread(false),
        mHardware(hw) { }
        virtual void onFirstRef() {
            run("CameraVideoThread", PRIORITY_DEFAULT);
        }
        virtual bool threadLoop() {
            mHardware->m_videoThreadFuncWrapper();
            return false;
        }
    };

    class PictureThread : public Thread {
        ExynosCameraHWInterface *mHardware;
    public:
        PictureThread(ExynosCameraHWInterface *hw):
        Thread(false),
        mHardware(hw) { }
        virtual bool threadLoop() {
            mHardware->m_pictureThreadFunc();
            return false;
        }
    };

    class AutoFocusThread : public Thread {
        ExynosCameraHWInterface *mHardware;
    public:
        AutoFocusThread(ExynosCameraHWInterface *hw): Thread(false), mHardware(hw) { }
        virtual void onFirstRef() {
            run("CameraAutoFocusThread", PRIORITY_DEFAULT);
        }
        virtual bool threadLoop() {
            mHardware->m_autoFocusThreadFunc();
            return true;
        }
    };

private:
    void        m_initDefaultParameters(int cameraId);

    bool        m_startPreviewInternal(void);
    void        m_stopPreviewInternal(void);

    bool        m_previewThreadFuncWrapper(void);
    bool        m_previewThreadFunc(void);
    bool        m_videoThreadFuncWrapper(void);
    bool        m_videoThreadFunc(void);
    bool        m_autoFocusThreadFunc(void);

    bool        m_startPictureInternal(void);
    bool        m_stopPictureInternal(void);
    bool        m_pictureThreadFunc(void);

    int         m_saveJpeg(unsigned char *real_jpeg, int jpeg_size);
    void        m_savePostView(const char *fname, uint8_t *buf,
                               uint32_t size);
    int         m_decodeInterleaveData(unsigned char *pInterleaveData,
                                       int interleaveDataSize,
                                       int yuvWidth,
                                       int yuvHeight,
                                       int *pJpegSize,
                                       void *pJpegData,
                                       void *pYuvData);
    bool        m_YUY2toNV21(void *srcBuf, void *dstBuf, uint32_t srcWidth, uint32_t srcHeight);
    bool        m_scaleDownYuv422(char *srcBuf, uint32_t srcWidth,
                                  uint32_t srcHight, char *dstBuf,
                                  uint32_t dstWidth, uint32_t dstHight);

    bool        m_checkVideoStartMarker(unsigned char *pBuf);
    bool        m_checkEOIMarker(unsigned char *pBuf);
    bool        m_findEOIMarkerInJPEG(unsigned char *pBuf,
                                      int dwBufSize, int *pnJPEGsize);
    bool        m_splitFrame(unsigned char *pFrame, int dwSize,
                             int dwJPEGLineLength, int dwVideoLineLength,
                             int dwVideoHeight, void *pJPEG,
                             int *pdwJPEGSize, void *pVideo,
                             int *pdwVideoSize);
    void        m_setSkipFrame(int frame);
    bool        m_isSupportedPreviewSize(const int width, const int height) const;

    void        m_getAlignedYUVSize(int colorFormat, int w, int h, ExynosBuffer *buf);

    bool        m_getResolutionList(String8 & string8Buf, char * strBuf, int w, int h);

    bool        m_getZoomRatioList(String8 & string8Buf, char * strBuf, int maxZoom, int start, int end);

    int         m_bracketsStr2Ints(char *str, int num, ExynosRect2 *rect2s, int *weights);
    bool        m_subBracketsStr2Ints(int num, char *str, int *arr);
    bool        m_getRatioSize(int  src_w,  int   src_h,
                               int  dst_w,  int   dst_h,
                               int *crop_x, int *crop_y,
                               int *crop_w, int *crop_h,
                               int zoom);
    int         m_calibratePosition(int w, int new_w, int x);
#ifdef LOG_NDEBUG
    bool        m_fileDump(char *filename, void *srcBuf, uint32_t size);
#endif

private:
    sp<PreviewThread>   m_previewThread;
    sp<VideoThread>     m_videoThread;
    sp<AutoFocusThread> m_autoFocusThread;
    sp<PictureThread>   m_pictureThread;

    /* used by auto focus thread to block until it's told to run */
    mutable Mutex       m_focusLock;
    mutable Condition   m_focusCondition;
            bool        m_exitAutoFocusThread;

    /* used by preview thread to block until it's told to run */
    mutable Mutex       m_previewLock;
    mutable Condition   m_previewCondition;
    mutable Condition   m_previewStoppedCondition;
            bool        m_previewRunning;
            bool        m_exitPreviewThread;
            bool        m_previewStartDeferred;

    mutable Mutex       m_videoLock;
    mutable Condition   m_videoCondition;
    mutable Condition   m_videoStoppedCondition;
            bool        m_videoRunning;
            bool        m_videoStart;
            bool        m_exitVideoThread;
            bool        m_recordingHint;

    void               *m_grallocVirtAddr[NUM_OF_PREVIEW_BUF];
    int                 m_matchedGrallocIndex[NUM_OF_PREVIEW_BUF];
    ExynosBuffer        m_pictureBuf;
    ExynosBuffer        copy_previewBuf;

    struct ExynosBufferQueue {
        ExynosBuffer       buf;
        ExynosBufferQueue *next;
    };

    ExynosBufferQueue  *m_oldPictureBufQueueHead;
    ExynosBufferQueue   m_oldPictureBufQueue[NUM_OF_PICTURE_BUF];
    mutable Mutex       m_pictureLock;
    mutable Condition   m_pictureCondition;
            bool        m_pictureRunning;
            bool        m_captureInProgress;
            int         m_numOfAvaliblePictureBuf;

    ExynosRect          m_orgPreviewRect;
    ExynosRect          m_orgPictureRect;
    ExynosRect          m_orgVideoRect;

    void               *m_exynosPreviewCSC;
    void               *m_exynosPictureCSC;
    void               *m_exynosVideoCSC;

            preview_stream_ops *m_previewWindow;

    /* used to guard threading state */
    mutable Mutex       m_stateLock;

    CameraParameters    m_params;
    CameraParameters    m_internalParams;

    camera_memory_t    *m_previewHeap[NUM_OF_PREVIEW_BUF];
    buffer_handle_t    *m_previewBufHandle[NUM_OF_PREVIEW_BUF];
    int                 m_previewStride[NUM_OF_PREVIEW_BUF];
    bool                m_avaliblePreviewBufHandle[NUM_OF_PREVIEW_BUF];
    bool                m_flagGrallocLocked[NUM_OF_PREVIEW_BUF];
    int                 m_minUndequeuedBufs;
    int                 m_numOfAvailableVideoBuf;
    int                 m_cntVideoBuf;

    camera_memory_t    *m_videoHeap[NUM_OF_VIDEO_BUF];
    camera_memory_t    *m_resizedVideoHeap[NUM_OF_VIDEO_BUF];
    camera_memory_t    *m_pictureHeap[NUM_OF_PICTURE_BUF];
    int			m_pictureFds[NUM_OF_PICTURE_BUF][3];
    camera_memory_t    *m_rawHeap;
    int			m_ion_client;

    camera_frame_metadata_t  m_frameMetadata;
    camera_face_t            m_faces[NUM_OF_DETECTED_FACES];
    bool                     m_faceDetected;

    ExynosCamera       *m_secCamera;

    mutable Mutex       m_skipFrameLock;
            int         m_skipFrame;

    camera_notify_callback     m_notifyCb;
    camera_data_callback       m_dataCb;
    camera_data_timestamp_callback m_dataCbTimestamp;
    camera_request_memory      m_getMemoryCb;
    void                      *m_callbackCookie;

            int32_t     m_msgEnabled;

           Vector<Size> m_supportedPreviewSizes;

    camera_device_t *m_halDevice;
    static gralloc_module_t const* m_grallocHal;
};

}; // namespace android

#endif
