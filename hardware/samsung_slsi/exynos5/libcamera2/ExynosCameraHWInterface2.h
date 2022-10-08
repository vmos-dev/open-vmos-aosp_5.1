/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2012, Samsung Electronics Co. LTD
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
 * \file      ExynosCameraHWInterface2.h
 * \brief     header file for Android Camera API 2.0 HAL
 * \author    Sungjoong Kang(sj3.kang@samsung.com)
 * \date      2012/07/10
 *
 * <b>Revision History: </b>
 * - 2012/05/31 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
  *
 * - 2012/07/10 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   2nd Release
 *
 */

#ifndef EXYNOS_CAMERA_HW_INTERFACE_2_H
#define EXYNOS_CAMERA_HW_INTERFACE_2_H

#include <hardware/camera2.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <utils/List.h>
#include "SignalDrivenThread.h"
#include "MetadataConverter.h"
#include "exynos_v4l2.h"
#include "ExynosRect.h"
#include "ExynosBuffer.h"
#include "videodev2_exynos_camera.h"
#include "gralloc_priv.h"
#include "ExynosJpegEncoderForCamera.h"
#include <fcntl.h>
#include "fimc-is-metadata.h"
#include "ion.h"
#include "ExynosExif.h"
#include "csc.h"
#include "ExynosCamera2.h"
#include "cutils/properties.h"

namespace android {

//#define EXYNOS_CAMERA_LOG
#define ENABLE_FRAME_SYNC
#define NODE_PREFIX     "/dev/video"

#define NUM_MAX_STREAM_THREAD       (5)
#define NUM_MAX_REQUEST_MGR_ENTRY   (5)
#define NUM_MAX_CAMERA_BUFFERS      (16)
#define NUM_BAYER_BUFFERS           (8)
#define NUM_SCC_BUFFERS             (8)
#define NUM_SCP_BUFFERS             (8)
#define NUM_MIN_SENSOR_QBUF         (3)
#define NUM_MAX_SUBSTREAM           (4)

#define PICTURE_GSC_NODE_NUM (2)
#define VIDEO_GSC_NODE_NUM (1)

#define STREAM_TYPE_DIRECT   (0)
#define STREAM_TYPE_INDIRECT (1)

#define SIGNAL_MAIN_REQ_Q_NOT_EMPTY             (SIGNAL_THREAD_COMMON_LAST<<1)

#define SIGNAL_MAIN_STREAM_OUTPUT_DONE          (SIGNAL_THREAD_COMMON_LAST<<3)
#define SIGNAL_SENSOR_START_REQ_PROCESSING      (SIGNAL_THREAD_COMMON_LAST<<4)

#define SIGNAL_THREAD_RELEASE                   (SIGNAL_THREAD_COMMON_LAST<<8)

#define SIGNAL_STREAM_REPROCESSING_START        (SIGNAL_THREAD_COMMON_LAST<<14)
#define SIGNAL_STREAM_DATA_COMING               (SIGNAL_THREAD_COMMON_LAST<<15)

#define NO_TRANSITION                   (0)
#define HAL_AFSTATE_INACTIVE            (1)
#define HAL_AFSTATE_NEEDS_COMMAND       (2)
#define HAL_AFSTATE_STARTED             (3)
#define HAL_AFSTATE_SCANNING            (4)
#define HAL_AFSTATE_LOCKED              (5)
#define HAL_AFSTATE_FAILED              (6)
#define HAL_AFSTATE_NEEDS_DETERMINATION (7)
#define HAL_AFSTATE_PASSIVE_FOCUSED     (8)

#define STREAM_ID_PREVIEW           (0)
#define STREAM_MASK_PREVIEW         (1<<STREAM_ID_PREVIEW)
#define STREAM_ID_RECORD            (1)
#define STREAM_MASK_RECORD          (1<<STREAM_ID_RECORD)
#define STREAM_ID_PRVCB             (2)
#define STREAM_MASK_PRVCB           (1<<STREAM_ID_PRVCB)
#define STREAM_ID_JPEG              (4)
#define STREAM_MASK_JPEG            (1<<STREAM_ID_JPEG)
#define STREAM_ID_ZSL               (5)
#define STREAM_MASK_ZSL             (1<<STREAM_ID_ZSL)

#define STREAM_ID_JPEG_REPROCESS    (8)
#define STREAM_ID_LAST              STREAM_ID_JPEG_REPROCESS

#define MASK_OUTPUT_SCP             (STREAM_MASK_PREVIEW|STREAM_MASK_RECORD|STREAM_MASK_PRVCB)
#define MASK_OUTPUT_SCC             (STREAM_MASK_JPEG|STREAM_MASK_ZSL)

#define SUBSTREAM_TYPE_NONE         (0)
#define SUBSTREAM_TYPE_JPEG         (1)
#define SUBSTREAM_TYPE_RECORD       (2)
#define SUBSTREAM_TYPE_PRVCB        (3)
#define FLASH_STABLE_WAIT_TIMEOUT        (10)

#define SIG_WAITING_TICK            (5000)

#ifdef EXYNOS_CAMERA_LOG
#define CAM_LOGV(...) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define CAM_LOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define CAM_LOGW(...) ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#define CAM_LOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#else
#define CAM_LOGV(...) ((void)0)
#define CAM_LOGD(...) ((void)0)
#define CAM_LOGW(...) ((void)0)
#define CAM_LOGE(...) ((void)0)
#endif

enum sensor_name {
    SENSOR_NAME_S5K3H2  = 1,
    SENSOR_NAME_S5K6A3  = 2,
    SENSOR_NAME_S5K4E5  = 3,
    SENSOR_NAME_S5K3H7  = 4,
    SENSOR_NAME_CUSTOM  = 5,
    SENSOR_NAME_END
};

enum is_subscenario_id {
	ISS_SUB_SCENARIO_STILL,
	ISS_SUB_SCENARIO_VIDEO,
	ISS_SUB_SCENARIO_SCENE1,
	ISS_SUB_SCENARIO_SCENE2,
	ISS_SUB_SCENARIO_SCENE3,
	ISS_SUB_END
};

enum is_set_flash_command_state {
    IS_FLASH_STATE_NONE = 0,
    IS_FLASH_STATE_ON = 1,
    IS_FLASH_STATE_ON_WAIT,
    IS_FLASH_STATE_ON_DONE,
    IS_FLASH_STATE_AUTO_AE_AWB_LOCK,
    IS_FLASH_STATE_AE_AWB_LOCK_WAIT,
    IS_FLASH_STATE_AUTO_WAIT,
    IS_FLASH_STATE_AUTO_DONE,
    IS_FLASH_STATE_AUTO_OFF,
    IS_FLASH_STATE_CAPTURE,
    IS_FLASH_STATE_CAPTURE_WAIT,
    IS_FLASH_STATE_CAPTURE_JPEG,
    IS_FLASH_STATE_CAPTURE_END,
    IS_FALSH_STATE_MAX
};

enum is_set_command_state {
    IS_COMMAND_NONE = 0,
    IS_COMMAND_EXECUTION,
    IS_COMMAND_CLEAR,
    IS_COMMAND_MAX
};

typedef struct node_info {
    int fd;
    int width;
    int height;
    int format;
    int planes;
    int buffers;
    enum v4l2_memory memory;
    enum v4l2_buf_type type;
    ExynosBuffer buffer[NUM_MAX_CAMERA_BUFFERS];
    int status;
} node_info_t;


typedef struct camera_hw_info {
    int sensor_id;

    node_info_t sensor;
    node_info_t isp;
    node_info_t capture;
    node_info_t scp;

    /*shot*/  // temp
    struct camera2_shot_ext dummy_shot;

} camera_hw_info_t;

typedef enum request_entry_status {
    EMPTY,
    REGISTERED,
    REQUESTED,
    CAPTURED,
    METADONE,
    COMPLETED
} request_entry_status_t;

typedef struct request_manager_entry {
    request_entry_status_t      status;
    camera_metadata_t           *original_request;
    struct camera2_shot_ext     internal_shot;
    int                         output_stream_count;
} request_manager_entry_t;

// structure related to a specific function of camera
typedef struct af_control_info {
    int    m_afTriggerTimeOut;
} ctl_af_info_t;

typedef struct flash_control_info {
    // UI flash mode indicator
    enum aa_aemode    i_flashMode;
    // AF flash
    bool        m_afFlashDoneFlg;
    // Capture flash
    bool        m_flashEnableFlg;
    int         m_flashFrameCount;
    int         m_flashCnt;
    int        m_flashTimeOut;
    // Flash decision
    // At flash auto mode only : 1 -> flash is needed, 0 -> normal case
    bool        m_flashDecisionResult;
    // torch indicator. this will be replaced by flashMode meta
    bool        m_flashTorchMode;
    // for precapture metering
    int        m_precaptureState;
    int        m_precaptureTriggerId;
} ctl_flash_info_t;

typedef struct ae_control_info {
    // pre-capture notification state
    enum ae_state    aeStateNoti;
} ctl_ae_info_t;

typedef struct scene_control_info {
    // pre-capture notification state
    enum aa_scene_mode    prevSceneMode;
} ctl_scene_info_t;

typedef struct request_control_info {
    ctl_flash_info_t flash;
    ctl_ae_info_t ae;
    ctl_af_info_t af;
    ctl_scene_info_t scene;
} ctl_request_info_t;

class RequestManager {
public:
    RequestManager(SignalDrivenThread* main_thread);
    ~RequestManager();
    void    ResetEntry();
    int     GetNumEntries();
    bool    IsRequestQueueFull();

    void    RegisterRequest(camera_metadata_t *new_request, int * afMode, uint32_t * afRegion);
    void    DeregisterRequest(camera_metadata_t **deregistered_request);
    bool    PrepareFrame(size_t *num_entries, size_t *frame_size,
                camera_metadata_t **prepared_frame, int afState);
    int     MarkProcessingRequest(ExynosBuffer * buf);
    void    NotifyStreamOutput(int frameCnt);
    void    ApplyDynamicMetadata(struct camera2_shot_ext *shot_ext);
    void    CheckCompleted(int index);
    void    UpdateIspParameters(struct camera2_shot_ext *shot_ext, int frameCnt, ctl_request_info_t *ctl_info);
    void    RegisterTimestamp(int frameCnt, nsecs_t *frameTime);
    nsecs_t  GetTimestampByFrameCnt(int frameCnt);
    nsecs_t  GetTimestamp(int index);
    uint8_t  GetOutputStreamByFrameCnt(int frameCnt);
    uint8_t  GetOutputStream(int index);
    camera2_shot_ext *  GetInternalShotExtByFrameCnt(int frameCnt);
    camera2_shot_ext *  GetInternalShotExt(int index);
    int     FindFrameCnt(struct camera2_shot_ext * shot_ext, bool drain);
    bool    IsVdisEnable(void);
    int     FindEntryIndexByFrameCnt(int frameCnt);
    void    Dump(void);
    int     GetNextIndex(int index);
    int     GetPrevIndex(int index);
    void    SetDefaultParameters(int cropX);
    void    SetInitialSkip(int count);
    int     GetSkipCnt();
    int     GetCompletedIndex();
    void    pushSensorQ(int index);
    int     popSensorQ();
    void    releaseSensorQ();

    bool    m_vdisEnable;

private:

    MetadataConverter               *m_metadataConverter;
    SignalDrivenThread              *m_mainThread;
    Mutex                           m_numOfEntriesLock;
    int                             m_numOfEntries;
    int                             m_entryInsertionIndex;
    int                             m_entryProcessingIndex;
    int                             m_entryFrameOutputIndex;
    request_manager_entry_t         entries[NUM_MAX_REQUEST_MGR_ENTRY];
    int                             m_completedIndex;

    Mutex                           m_requestMutex;

    //TODO : alloc dynamically
    char                            m_tempFrameMetadataBuf[2000];
    camera_metadata_t               *m_tempFrameMetadata;

    int                             m_sensorPipelineSkipCnt;
    int                             m_cropX;
    int                             m_lastCompletedFrameCnt;
    int                             m_lastAeMode;
    int                             m_lastAaMode;
    int                             m_lastAwbMode;
    int                             m_lastAeComp;
    bool                            m_vdisBubbleEn;
    nsecs_t                         m_lastTimeStamp;
    List<int>                   m_sensorQ;
};


typedef struct bayer_buf_entry {
    int     status;
    int     reqFrameCnt;
    nsecs_t timeStamp;
} bayer_buf_entry_t;


class BayerBufManager {
public:
    BayerBufManager();
    ~BayerBufManager();
    int                 GetIndexForSensorEnqueue();
    int                 MarkSensorEnqueue(int index);
    int                 MarkSensorDequeue(int index, int reqFrameCnt, nsecs_t *timeStamp);
    int                 GetIndexForIspEnqueue(int *reqFrameCnt);
    int                 GetIndexForIspDequeue(int *reqFrameCnt);
    int                 MarkIspEnqueue(int index);
    int                 MarkIspDequeue(int index);
    int                 GetNumOnSensor();
    int                 GetNumOnHalFilled();
    int                 GetNumOnIsp();

private:
    int                 GetNextIndex(int index);

    int                 sensorEnqueueHead;
    int                 sensorDequeueHead;
    int                 ispEnqueueHead;
    int                 ispDequeueHead;
    int                 numOnSensor;
    int                 numOnIsp;
    int                 numOnHalFilled;
    int                 numOnHalEmpty;

    bayer_buf_entry_t   entries[NUM_BAYER_BUFFERS];
};


#define NOT_AVAILABLE           (0)
#define REQUIRES_DQ_FROM_SVC    (1)
#define ON_DRIVER               (2)
#define ON_HAL                  (3)
#define ON_SERVICE              (4)

#define BAYER_NOT_AVAILABLE     (0)
#define BAYER_ON_SENSOR         (1)
#define BAYER_ON_HAL_FILLED     (2)
#define BAYER_ON_ISP            (3)
#define BAYER_ON_SERVICE        (4)
#define BAYER_ON_HAL_EMPTY      (5)

typedef struct stream_parameters {
            uint32_t                width;
            uint32_t                height;
            int                     format;
    const   camera2_stream_ops_t*   streamOps;
            uint32_t                usage;
            int                     numHwBuffers;
            int                     numSvcBuffers;
            int                     numOwnSvcBuffers;
            int                     planes;
            int                     metaPlanes;
            int                     numSvcBufsInHal;
            buffer_handle_t         svcBufHandle[NUM_MAX_CAMERA_BUFFERS];
            ExynosBuffer            svcBuffers[NUM_MAX_CAMERA_BUFFERS];
            ExynosBuffer            metaBuffers[NUM_MAX_CAMERA_BUFFERS];
            int                     svcBufStatus[NUM_MAX_CAMERA_BUFFERS];
            int                     bufIndex;
            node_info_t             *node;
            int                     minUndequedBuffer;
            bool                    needsIonMap;
} stream_parameters_t;

typedef struct substream_parameters {
            int                     type;
            uint32_t                width;
            uint32_t                height;
            int                     format;
    const   camera2_stream_ops_t*   streamOps;
            uint32_t                usage;
            int                     numSvcBuffers;
            int                     numOwnSvcBuffers;
            int                     internalFormat;
            int                     internalPlanes;
            int                     svcPlanes;
            buffer_handle_t         svcBufHandle[NUM_MAX_CAMERA_BUFFERS];
            ExynosBuffer            svcBuffers[NUM_MAX_CAMERA_BUFFERS];
            int                     svcBufStatus[NUM_MAX_CAMERA_BUFFERS];
            int                     svcBufIndex;
            int                     numSvcBufsInHal;
            bool                    needBufferInit;
            int                     minUndequedBuffer;
} substream_parameters_t;

typedef struct substream_entry {
    int                     priority;
    int                     streamId;
} substream_entry_t;

class ExynosCameraHWInterface2 : public virtual RefBase {
public:
    ExynosCameraHWInterface2(int cameraId, camera2_device_t *dev, ExynosCamera2 * camera, int *openInvalid);
    virtual             ~ExynosCameraHWInterface2();

    virtual void        release();

    inline  int         getCameraId() const;

    virtual int         setRequestQueueSrcOps(const camera2_request_queue_src_ops_t *request_src_ops);
    virtual int         notifyRequestQueueNotEmpty();
    virtual int         setFrameQueueDstOps(const camera2_frame_queue_dst_ops_t *frame_dst_ops);
    virtual int         getInProgressCount();
    virtual int         flushCapturesInProgress();
    virtual int         constructDefaultRequest(int request_template, camera_metadata_t **request);
    virtual int         allocateStream(uint32_t width, uint32_t height,
                                    int format, const camera2_stream_ops_t *stream_ops,
                                    uint32_t *stream_id, uint32_t *format_actual, uint32_t *usage, uint32_t *max_buffers);
    virtual int         registerStreamBuffers(uint32_t stream_id, int num_buffers, buffer_handle_t *buffers);
    virtual int         releaseStream(uint32_t stream_id);
    virtual int         allocateReprocessStream(uint32_t width, uint32_t height,
                                    uint32_t format, const camera2_stream_in_ops_t *reprocess_stream_ops,
                                    uint32_t *stream_id, uint32_t *consumer_usage, uint32_t *max_buffers);
    virtual int         allocateReprocessStreamFromStream(uint32_t output_stream_id,
                                const camera2_stream_in_ops_t *reprocess_stream_ops, uint32_t *stream_id);
    virtual int         releaseReprocessStream(uint32_t stream_id);
    virtual int         triggerAction(uint32_t trigger_id, int ext1, int ext2);
    virtual int         setNotifyCallback(camera2_notify_callback notify_cb, void *user);
    virtual int         getMetadataVendorTagOps(vendor_tag_query_ops_t **ops);
    virtual int         dump(int fd);
private:
class MainThread : public SignalDrivenThread {
        ExynosCameraHWInterface2 *mHardware;
    public:
        MainThread(ExynosCameraHWInterface2 *hw):
            SignalDrivenThread(),
            mHardware(hw) { }
        ~MainThread();
        void threadFunctionInternal()
	    {
            mHardware->m_mainThreadFunc(this);
            return;
        }
        void        release(void);
        bool        m_releasing;
    };

    class SensorThread : public SignalDrivenThread {
        ExynosCameraHWInterface2 *mHardware;
    public:
        SensorThread(ExynosCameraHWInterface2 *hw):
            SignalDrivenThread(),
            mHardware(hw) { }
        ~SensorThread();
        void threadFunctionInternal() {
            mHardware->m_sensorThreadFunc(this);
            return;
        }
        void            release(void);
    //private:
        bool            m_releasing;
    };

    class StreamThread : public SignalDrivenThread {
        ExynosCameraHWInterface2 *mHardware;
    public:
        StreamThread(ExynosCameraHWInterface2 *hw, uint8_t new_index):
            SignalDrivenThread(),
            mHardware(hw),
            m_index(new_index) { }
        ~StreamThread();
        void threadFunctionInternal() {
            mHardware->m_streamThreadFunc(this);
            return;
        }
        void        setParameter(stream_parameters_t * new_parameters);
        status_t    attachSubStream(int stream_id, int priority);
        status_t    detachSubStream(int stream_id);
        void        release(void);
        int         findBufferIndex(void * bufAddr);
        int         findBufferIndex(buffer_handle_t * bufHandle);

        uint8_t                         m_index;
        bool                            m_activated;
    //private:
        stream_parameters_t             m_parameters;
        stream_parameters_t             *m_tempParameters;
        substream_entry_t               m_attachedSubStreams[NUM_MAX_SUBSTREAM];
        bool                            m_isBufferInit;
        bool                            m_releasing;
        int                             streamType;
        int                             m_numRegisteredStream;
     };

    sp<MainThread>      m_mainThread;
    sp<SensorThread>    m_sensorThread;
    sp<StreamThread>    m_streamThreads[NUM_MAX_STREAM_THREAD];
    substream_parameters_t  m_subStreams[STREAM_ID_LAST+1];



    RequestManager      *m_requestManager;
    BayerBufManager     *m_BayerManager;
    ExynosCamera2       *m_camera2;

    void                m_mainThreadFunc(SignalDrivenThread * self);
    void                m_sensorThreadFunc(SignalDrivenThread * self);
    void                m_streamThreadFunc(SignalDrivenThread * self);
    void                m_streamThreadInitialize(SignalDrivenThread * self);

    void                m_streamFunc_direct(SignalDrivenThread *self);
    void                m_streamFunc_indirect(SignalDrivenThread *self);

    void                m_streamBufferInit(SignalDrivenThread *self);

    int                 m_runSubStreamFunc(StreamThread *selfThread, ExynosBuffer *srcImageBuf,
                            int stream_id, nsecs_t frameTimeStamp);
    int                 m_jpegCreator(StreamThread *selfThread, ExynosBuffer *srcImageBuf, nsecs_t frameTimeStamp);
    int                 m_recordCreator(StreamThread *selfThread, ExynosBuffer *srcImageBuf, nsecs_t frameTimeStamp);
    int                 m_prvcbCreator(StreamThread *selfThread, ExynosBuffer *srcImageBuf, nsecs_t frameTimeStamp);
    void                m_getAlignedYUVSize(int colorFormat, int w, int h,
                                                ExynosBuffer *buf);
    bool                m_getRatioSize(int  src_w,  int   src_h,
                                             int  dst_w,  int   dst_h,
                                             int *crop_x, int *crop_y,
                                             int *crop_w, int *crop_h,
                                             int zoom);
	int				createIonClient(ion_client ionClient);
	int					deleteIonClient(ion_client ionClient);

    int				allocCameraMemory(ion_client ionClient, ExynosBuffer *buf, int iMemoryNum);
    int             allocCameraMemory(ion_client ionClient, ExynosBuffer *buf, int iMemoryNum, int cacheFlag);
	void				freeCameraMemory(ExynosBuffer *buf, int iMemoryNum);
	void				initCameraMemory(ExynosBuffer *buf, int iMemoryNum);

    void            DumpInfoWithShot(struct camera2_shot_ext * shot_ext);
    bool            m_checkThumbnailSize(int w, int h);
    bool            yuv2Jpeg(ExynosBuffer *yuvBuf,
                            ExynosBuffer *jpegBuf,
                            ExynosRect *rect);
    int             InitializeISPChain();
    void            StartISP();
    void            StartSCCThread(bool threadExists);
    int             GetAfState();
    void            SetAfMode(enum aa_afmode afMode);
    void            SetAfRegion(uint32_t * afRegion);
    void            OnAfTrigger(int id);
    void            OnAfTriggerAutoMacro(int id);
    void            OnAfTriggerCAFPicture(int id);
    void            OnAfTriggerCAFVideo(int id);
    void            OnPrecaptureMeteringTriggerStart(int id);
    void            OnAfCancel(int id);
    void            OnAfCancelAutoMacro(int id);
    void            OnAfCancelCAFPicture(int id);
    void            OnAfCancelCAFVideo(int id);
    void            OnPrecaptureMeteringNotificationISP();
    void            OnPrecaptureMeteringNotificationSensor();
    void            OnAfNotification(enum aa_afstate noti);
    void            OnAfNotificationAutoMacro(enum aa_afstate noti);
    void            OnAfNotificationCAFPicture(enum aa_afstate noti);
    void            OnAfNotificationCAFVideo(enum aa_afstate noti);
    void            SetAfStateForService(int newState);
    int             GetAfStateForService();
    exif_attribute_t    mExifInfo;
    void            m_setExifFixedAttribute(void);
    void            m_setExifChangedAttribute(exif_attribute_t *exifInfo, ExynosRect *rect,
                         camera2_shot_ext *currentEntry);
    void            m_preCaptureSetter(struct camera2_shot_ext * shot_ext);
    void            m_preCaptureListenerSensor(struct camera2_shot_ext * shot_ext);
    void            m_preCaptureListenerISP(struct camera2_shot_ext * shot_ext);
    void            m_preCaptureAeState(struct camera2_shot_ext * shot_ext);
    void            m_updateAfRegion(struct camera2_shot_ext * shot_ext);
    void            m_afTrigger(struct camera2_shot_ext * shot_ext, int mode);
    void               *m_exynosPictureCSC;
    void               *m_exynosVideoCSC;


    camera2_request_queue_src_ops_t     *m_requestQueueOps;
    camera2_frame_queue_dst_ops_t       *m_frameQueueOps;
    camera2_notify_callback             m_notifyCb;
    void                                *m_callbackCookie;

    int                                 m_numOfRemainingReqInSvc;
    bool                                m_isRequestQueuePending;
    bool                                m_isRequestQueueNull;
    camera2_device_t                    *m_halDevice;
    static gralloc_module_t const*      m_grallocHal;


    camera_hw_info_t                     m_camera_info;

	ion_client m_ionCameraClient;

    bool                                m_isIspStarted;

    int                                 m_need_streamoff;
    ExynosBuffer                        m_sccLocalBuffer[NUM_MAX_CAMERA_BUFFERS];
    bool                                m_sccLocalBufferValid;

    int                                 indexToQueue[3+1];

    bool                                m_scp_flushing;
    bool                                m_closing;
    ExynosBuffer                        m_resizeBuf;
#ifndef ENABLE_FRAME_SYNC
    int                                 m_currentOutputStreams;
#endif
    int                                 m_currentReprocessOutStreams;
    ExynosBuffer                        m_previewCbBuf;
    int             				    m_cameraId;
    bool                                m_scp_closing;
    bool                                m_scp_closed;
    bool                                m_wideAspect;
    uint32_t                            currentAfRegion[4];
    float                               m_zoomRatio;

    int                                 m_vdisBubbleCnt;
    int                                 m_vdisDupFrame;

    mutable Mutex                       m_qbufLock;
    mutable Mutex                       m_jpegEncoderLock;
    int                                 m_jpegEncodingCount;
    mutable Mutex                       m_afModeTriggerLock;

    bool                                m_scpForceSuspended;
    int                                 m_afState;
    int                                 m_afTriggerId;
    enum aa_afmode                      m_afMode;
    enum aa_afmode                      m_afMode2;
    bool                                m_IsAfModeUpdateRequired;
    bool                                m_IsAfTriggerRequired;
    bool                                m_IsAfLockRequired;
    int                                 m_serviceAfState;
    bool                                m_AfHwStateFailed;
    int                                 m_afPendingTriggerId;
    int                                 m_afModeWaitingCnt;
    struct camera2_shot_ext             m_jpegMetadata;
    int                                 m_scpOutputSignalCnt;
    int                                 m_scpOutputImageCnt;
    int                                 m_nightCaptureCnt;
    int                                 m_nightCaptureFrameCnt;
    int                                 m_lastSceneMode;
    int                                 m_thumbNailW;
    int                                 m_thumbNailH;
    int                                 m_reprocessStreamId;
    const camera2_stream_in_ops_t *     m_reprocessOps;
    int                                 m_reprocessOutputStreamId;
    int                                 m_reprocessingFrameCnt;
    ctl_request_info_t        m_ctlInfo;
};

}; // namespace android

#endif
