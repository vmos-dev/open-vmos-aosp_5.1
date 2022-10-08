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

#ifndef __QCAMERA2HARDWAREINTERFACE_H__
#define __QCAMERA2HARDWAREINTERFACE_H__

#include <hardware/camera.h>
#include <hardware/power.h>
#include <utils/Log.h>
#include <QCameraParameters.h>

#include "QCameraQueue.h"
#include "QCameraCmdThread.h"
#include "QCameraChannel.h"
#include "QCameraStream.h"
#include "QCameraStateMachine.h"
#include "QCameraAllocator.h"
#include "QCameraPostProc.h"
#include "QCameraThermalAdapter.h"

extern "C" {
#include <mm_camera_interface.h>
#include <mm_jpeg_interface.h>
}

#if DISABLE_DEBUG_LOG

inline void __null_log(int, const char *, const char *, ...) {}

#ifdef ALOGD
#undef ALOGD
#define ALOGD(...) do { __null_log(0, LOG_TAG,__VA_ARGS__); } while (0)
#endif

#ifdef ALOGI
#undef ALOGI
#define ALOGI(...) do { __null_log(0, LOG_TAG,__VA_ARGS__); } while (0)
#endif

#endif

namespace qcamera {

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef enum {
    QCAMERA_CH_TYPE_ZSL,
    QCAMERA_CH_TYPE_CAPTURE,
    QCAMERA_CH_TYPE_PREVIEW,
    QCAMERA_CH_TYPE_VIDEO,
    QCAMERA_CH_TYPE_SNAPSHOT,
    QCAMERA_CH_TYPE_RAW,
    QCAMERA_CH_TYPE_METADATA,
    QCAMERA_CH_TYPE_MAX
} qcamera_ch_type_enum_t;

typedef struct {
    int32_t msg_type;
    int32_t ext1;
    int32_t ext2;
} qcamera_evt_argm_t;

#define QCAMERA_DUMP_FRM_PREVIEW    1
#define QCAMERA_DUMP_FRM_VIDEO      (1<<1)
#define QCAMERA_DUMP_FRM_SNAPSHOT   (1<<2)
#define QCAMERA_DUMP_FRM_THUMBNAIL  (1<<3)
#define QCAMERA_DUMP_FRM_RAW        (1<<4)
#define QCAMERA_DUMP_FRM_JPEG       (1<<5)

#define QCAMERA_DUMP_FRM_MASK_ALL    0x000000ff

#define QCAMERA_ION_USE_CACHE   true
#define QCAMERA_ION_USE_NOCACHE false

typedef enum {
    QCAMERA_NOTIFY_CALLBACK,
    QCAMERA_DATA_CALLBACK,
    QCAMERA_DATA_TIMESTAMP_CALLBACK,
    QCAMERA_DATA_SNAPSHOT_CALLBACK
} qcamera_callback_type_m;

typedef void (*camera_release_callback)(void *user_data, void *cookie);

typedef struct {
    qcamera_callback_type_m  cb_type;    // event type
    int32_t                  msg_type;   // msg type
    int32_t                  ext1;       // extended parameter
    int32_t                  ext2;       // extended parameter
    camera_memory_t *        data;       // ptr to data memory struct
    unsigned int             index;      // index of the buf in the whole buffer
    int64_t                  timestamp;  // buffer timestamp
    camera_frame_metadata_t *metadata;   // meta data
    void                    *user_data;  // any data needs to be released after callback
    void                    *cookie;     // release callback cookie
    camera_release_callback  release_cb; // release callback
} qcamera_callback_argm_t;

class QCameraCbNotifier {
public:
    QCameraCbNotifier(QCamera2HardwareInterface *parent) :
                          mNotifyCb (NULL),
                          mDataCb (NULL),
                          mDataCbTimestamp (NULL),
                          mCallbackCookie (NULL),
                          mParent (parent),
                          mDataQ(releaseNotifications, this) {}

    virtual ~QCameraCbNotifier();

    virtual int32_t notifyCallback(qcamera_callback_argm_t &cbArgs);
    virtual void setCallbacks(camera_notify_callback notifyCb,
                              camera_data_callback dataCb,
                              camera_data_timestamp_callback dataCbTimestamp,
                              void *callbackCookie);
    virtual int32_t startSnapshots();
    virtual void stopSnapshots();
    static void * cbNotifyRoutine(void * data);
    static void releaseNotifications(void *data, void *user_data);
    static bool matchSnapshotNotifications(void *data, void *user_data);
private:

    camera_notify_callback         mNotifyCb;
    camera_data_callback           mDataCb;
    camera_data_timestamp_callback mDataCbTimestamp;
    void                          *mCallbackCookie;
    QCamera2HardwareInterface     *mParent;

    QCameraQueue     mDataQ;
    QCameraCmdThread mProcTh;
};
class QCamera2HardwareInterface : public QCameraAllocator,
                                    public QCameraThermalCallback
{
public:
    /* static variable and functions accessed by camera service */
    static camera_device_ops_t mCameraOps;

    static int set_preview_window(struct camera_device *,
        struct preview_stream_ops *window);
    static void set_CallBacks(struct camera_device *,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user);
    static void enable_msg_type(struct camera_device *, int32_t msg_type);
    static void disable_msg_type(struct camera_device *, int32_t msg_type);
    static int msg_type_enabled(struct camera_device *, int32_t msg_type);
    static int start_preview(struct camera_device *);
    static void stop_preview(struct camera_device *);
    static int preview_enabled(struct camera_device *);
    static int store_meta_data_in_buffers(struct camera_device *, int enable);
    static int start_recording(struct camera_device *);
    static void stop_recording(struct camera_device *);
    static int recording_enabled(struct camera_device *);
    static void release_recording_frame(struct camera_device *, const void *opaque);
    static int auto_focus(struct camera_device *);
    static int cancel_auto_focus(struct camera_device *);
    static int take_picture(struct camera_device *);
    static int cancel_picture(struct camera_device *);
    static int set_parameters(struct camera_device *, const char *parms);
    static char* get_parameters(struct camera_device *);
    static void put_parameters(struct camera_device *, char *);
    static int send_command(struct camera_device *,
              int32_t cmd, int32_t arg1, int32_t arg2);
    static void release(struct camera_device *);
    static int dump(struct camera_device *, int fd);
    static int close_camera_device(hw_device_t *);

    static int register_face_image(struct camera_device *,
                                   void *img_ptr,
                                   cam_pp_offline_src_config_t *config);
public:
    QCamera2HardwareInterface(int cameraId);
    virtual ~QCamera2HardwareInterface();
    int openCamera(struct hw_device_t **hw_device);

    static int getCapabilities(int cameraId, struct camera_info *info);
    static int initCapabilities(int cameraId);

    // Implementation of QCameraAllocator
    virtual QCameraMemory *allocateStreamBuf(cam_stream_type_t stream_type,
                                             int size,
                                             uint8_t &bufferCnt);
    virtual QCameraHeapMemory *allocateStreamInfoBuf(cam_stream_type_t stream_type);

    // Implementation of QCameraThermalCallback
    virtual int thermalEvtHandle(qcamera_thermal_level_enum_t level,
            void *userdata, void *data);

    friend class QCameraStateMachine;
    friend class QCameraPostProcessor;
    friend class QCameraCbNotifier;

private:
    int setPreviewWindow(struct preview_stream_ops *window);
    int setCallBacks(
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user);
    int enableMsgType(int32_t msg_type);
    int disableMsgType(int32_t msg_type);
    int msgTypeEnabled(int32_t msg_type);
    int msgTypeEnabledWithLock(int32_t msg_type);
    int startPreview();
    int stopPreview();
    int storeMetaDataInBuffers(int enable);
    int startRecording();
    int stopRecording();
    int releaseRecordingFrame(const void *opaque);
    int autoFocus();
    int cancelAutoFocus();
    int takePicture();
    int cancelPicture();
    int takeLiveSnapshot();
    int cancelLiveSnapshot();
    char* getParameters();
    int putParameters(char *);
    int sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);
    int release();
    int dump(int fd);
    int registerFaceImage(void *img_ptr,
                          cam_pp_offline_src_config_t *config,
                          int32_t &faceID);

    int openCamera();
    int closeCamera();

    int processAPI(qcamera_sm_evt_enum_t api, void *api_payload);
    int processEvt(qcamera_sm_evt_enum_t evt, void *evt_payload);
    int processSyncEvt(qcamera_sm_evt_enum_t evt, void *evt_payload);
    void lockAPI();
    void waitAPIResult(qcamera_sm_evt_enum_t api_evt);
    void unlockAPI();
    void signalAPIResult(qcamera_api_result_t *result);
    void signalEvtResult(qcamera_api_result_t *result);

    int updateThermalLevel(qcamera_thermal_level_enum_t level);

    // update entris to set parameters and check if restart is needed
    int updateParameters(const char *parms, bool &needRestart);
    // send request to server to set parameters
    int commitParameterChanges();

    bool needDebugFps();
    bool isCACEnabled();
    bool needReprocess();
    bool needRotationReprocess();
    void debugShowVideoFPS();
    void debugShowPreviewFPS();
    void dumpFrameToFile(const void *data, uint32_t size,
                         int index, int dump_type);
    void releaseSuperBuf(mm_camera_super_buf_t *super_buf);
    void playShutter();
    void getThumbnailSize(cam_dimension_t &dim);
    int getJpegQuality();
    int getJpegRotation();
    QCameraExif *getExifData();

    int32_t processAutoFocusEvent(cam_auto_focus_data_t &focus_data);
    int32_t processZoomEvent(cam_crop_data_t &crop_info);
    int32_t processPrepSnapshotDoneEvent(cam_prep_snapshot_state_t prep_snapshot_state);
    int32_t processJpegNotify(qcamera_jpeg_evt_payload_t *jpeg_job);

    int32_t sendEvtNotify(int32_t msg_type, int32_t ext1, int32_t ext2);
    int32_t sendDataNotify(int32_t msg_type,
                           camera_memory_t *data,
                           uint8_t index,
                           camera_frame_metadata_t *metadata);

    int32_t addChannel(qcamera_ch_type_enum_t ch_type);
    int32_t startChannel(qcamera_ch_type_enum_t ch_type);
    int32_t stopChannel(qcamera_ch_type_enum_t ch_type);
    int32_t delChannel(qcamera_ch_type_enum_t ch_type);
    int32_t addPreviewChannel();
    int32_t addSnapshotChannel();
    int32_t addVideoChannel();
    int32_t addZSLChannel();
    int32_t addCaptureChannel();
    int32_t addRawChannel();
    int32_t addMetaDataChannel();
    QCameraReprocessChannel *addOnlineReprocChannel(QCameraChannel *pInputChannel);
    QCameraReprocessChannel *addOfflineReprocChannel(
                                                cam_pp_offline_src_config_t &img_config,
                                                cam_pp_feature_config_t &pp_feature,
                                                stream_cb_routine stream_cb,
                                                void *userdata);
    int32_t addStreamToChannel(QCameraChannel *pChannel,
                               cam_stream_type_t streamType,
                               stream_cb_routine streamCB,
                               void *userData);
    int32_t preparePreview();
    void unpreparePreview();
    QCameraChannel *getChannelByHandle(uint32_t channelHandle);
    mm_camera_buf_def_t *getSnapshotFrame(mm_camera_super_buf_t *recvd_frame);
    int32_t processFaceDetectionResult(cam_face_detection_data_t *fd_data);
    int32_t processHistogramStats(cam_hist_stats_t &stats_data);
    int32_t setHistogram(bool histogram_en);
    int32_t setFaceDetection(bool enabled);
    int32_t prepareHardwareForSnapshot(int32_t afNeeded);
    bool needProcessPreviewFrame() {return m_stateMachine.isPreviewRunning();};
    bool isNoDisplayMode() {return mParameters.isNoDisplayMode();};
    bool isZSLMode() {return mParameters.isZSLMode();};
    uint8_t numOfSnapshotsExpected() {return mParameters.getNumOfSnapshots();};
    uint8_t getBufNumRequired(cam_stream_type_t stream_type);

    static void camEvtHandle(uint32_t camera_handle,
                          mm_camera_event_t *evt,
                          void *user_data);
    static void jpegEvtHandle(jpeg_job_status_t status,
                              uint32_t client_hdl,
                              uint32_t jobId,
                              mm_jpeg_output_t *p_buf,
                              void *userdata);

    static void *evtNotifyRoutine(void *data);

    // functions for different data notify cb
    static void zsl_channel_cb(mm_camera_super_buf_t *recvd_frame, void *userdata);
    static void capture_channel_cb_routine(mm_camera_super_buf_t *recvd_frame,
                                           void *userdata);
    static void postproc_channel_cb_routine(mm_camera_super_buf_t *recvd_frame,
                                            void *userdata);
    static void nodisplay_preview_stream_cb_routine(mm_camera_super_buf_t *frame,
                                                    QCameraStream *stream,
                                                    void *userdata);
    static void preview_stream_cb_routine(mm_camera_super_buf_t *frame,
                                          QCameraStream *stream,
                                          void *userdata);
    static void postview_stream_cb_routine(mm_camera_super_buf_t *frame,
                                           QCameraStream *stream,
                                           void *userdata);
    static void video_stream_cb_routine(mm_camera_super_buf_t *frame,
                                        QCameraStream *stream,
                                        void *userdata);
    static void snapshot_stream_cb_routine(mm_camera_super_buf_t *frame,
                                           QCameraStream *stream,
                                           void *userdata);
    static void raw_stream_cb_routine(mm_camera_super_buf_t *frame,
                                      QCameraStream *stream,
                                      void *userdata);
    static void metadata_stream_cb_routine(mm_camera_super_buf_t *frame,
                                           QCameraStream *stream,
                                           void *userdata);
    static void reprocess_stream_cb_routine(mm_camera_super_buf_t *frame,
                                            QCameraStream *stream,
                                            void *userdata);

    static void releaseCameraMemory(void *data, void *cookie);
    static void returnStreamBuffer(void *data, void *cookie);

private:
    camera_device_t   mCameraDevice;
    uint8_t           mCameraId;
    mm_camera_vtbl_t *mCameraHandle;
    bool mCameraOpened;

    preview_stream_ops_t *mPreviewWindow;
    QCameraParameters mParameters;
    int32_t               mMsgEnabled;
    int                   mStoreMetaDataInFrame;

    camera_notify_callback         mNotifyCb;
    camera_data_callback           mDataCb;
    camera_data_timestamp_callback mDataCbTimestamp;
    camera_request_memory          mGetMemory;
    void                          *mCallbackCookie;

    QCameraStateMachine m_stateMachine;   // state machine
    QCameraPostProcessor m_postprocessor; // post processor
    QCameraThermalAdapter &m_thermalAdapter;
    QCameraCbNotifier m_cbNotifier;
    pthread_mutex_t m_lock;
    pthread_cond_t m_cond;
    qcamera_api_result_t m_apiResult;

    pthread_mutex_t m_evtLock;
    pthread_cond_t m_evtCond;
    qcamera_api_result_t m_evtResult;

    QCameraChannel *m_channels[QCAMERA_CH_TYPE_MAX]; // array holding channel ptr

    bool m_bShutterSoundPlayed;         // if shutter sound had been played

    // if auto focus is running, in other words, when auto_focus is called from service,
    // and beforeany focus callback/cancel_focus happens. This flag is not an indication
    // of whether lens is moving or not.
    bool m_bAutoFocusRunning;
    cam_autofocus_state_t m_currentFocusState;

    // If start_zsl_snapshot is called to notify camera daemon about zsl snapshot
    bool m_bStartZSLSnapshotCalled;

    power_module_t *m_pPowerModule;   // power module

    int mDumpFrmCnt;  // frame dump count
    int mDumpSkipCnt; // frame skip count
};

}; // namespace qcamera

#endif /* __QCAMERA2HARDWAREINTERFACE_H__ */
