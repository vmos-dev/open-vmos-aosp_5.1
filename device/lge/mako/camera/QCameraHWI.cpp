/*
** Copyright (c) 2011-2012,2015 The Linux Foundation. All rights reserved.
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

/*#error uncomment this for compiler test!*/
#define ALOG_NIDEBUG 0

#define LOG_TAG "QCameraHWI"
#include <utils/Log.h>
#include <utils/threads.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>

#include "QCameraHAL.h"
#include "QCameraHWI.h"

/* QCameraHardwareInterface class implementation goes here*/
/* following code implement the contol logic of this class*/

namespace android {
static void HAL_event_cb(mm_camera_event_t *evt, void *user_data)
{
  QCameraHardwareInterface *obj = (QCameraHardwareInterface *)user_data;
  if (obj) {
    obj->processEvent(evt);
  } else {
    ALOGE("%s: NULL user_data", __func__);
  }
}

int32_t QCameraHardwareInterface::createRecord()
{
    int32_t ret = MM_CAMERA_OK;
    ALOGV("%s : BEGIN",__func__);

    /*
    * Creating Instance of record stream.
    */
    ALOGV("Mymode Record = %d",myMode);
    mStreamRecord = QCameraStream_record::createInstance(mCameraId,
                                                         myMode);

    if (!mStreamRecord) {
        ALOGE("%s: error - can't creat record stream!", __func__);
        return BAD_VALUE;
    }

    /* Store HAL object in record stream Object */
    mStreamRecord->setHALCameraControl(this);

    /*Init Channel */
    ret = mStreamRecord->init();
    if (MM_CAMERA_OK != ret){
        ALOGE("%s: error - can't init Record channel!", __func__);
        return BAD_VALUE;
    }
    ALOGV("%s : END",__func__);
    return ret;
}

int32_t QCameraHardwareInterface::createSnapshot()
{
    int32_t ret = MM_CAMERA_OK;
    ALOGV("%s : BEGIN",__func__);

    /*
    * Creating Instance of Snapshot stream.
    */
    ALOGV("Mymode Snap = %d",myMode);
    mStreamSnap = QCameraStream_Snapshot::createInstance(mCameraId,
                                                         myMode);
    if (!mStreamSnap) {
        ALOGE("%s: error - can't creat snapshot stream!", __func__);
        return BAD_VALUE;
    }

    /* Store HAL object in Snapshot stream Object */
    mStreamSnap->setHALCameraControl(this);

    /*Init Channel */
    ret = mStreamSnap->init();
    if (MM_CAMERA_OK != ret){
        ALOGE("%s: error - can't init Snapshot channel!", __func__);
        return BAD_VALUE;
    }
    ALOGV("%s : END",__func__);
    return ret;
}

int32_t QCameraHardwareInterface::createPreview()
{
    int32_t ret = MM_CAMERA_OK;
    ALOGV("%s : BEGIN",__func__);

    ALOGV("Mymode Preview = %d",myMode);
    mStreamDisplay = QCameraStream_preview::createInstance(mCameraId,
                                                           myMode);
    if (!mStreamDisplay) {
        ALOGE("%s: error - can't creat preview stream!", __func__);
        return BAD_VALUE;
    }

    mStreamDisplay->setHALCameraControl(this);

    /*now init all the buffers and send to steam object*/
    ret = mStreamDisplay->init();
    if (MM_CAMERA_OK != ret){
        ALOGE("%s: error - can't init Preview channel!", __func__);
        return BAD_VALUE;
    }
    ALOGV("%s : END",__func__);
    return ret;
}

/* constructor */
QCameraHardwareInterface::
QCameraHardwareInterface(int cameraId, int mode)
                  : mCameraId(cameraId),
                    mParameters(),
                    mMsgEnabled(0),
                    mNotifyCb(0),
                    mDataCb(0),
                    mDataCbTimestamp(0),
                    mCallbackCookie(0),
                    //mPreviewHeap(0),
                    mStreamDisplay (NULL), mStreamRecord(NULL), mStreamSnap(NULL),
                    mFps(0),
                    mDebugFps(0),
                    mMaxZoom(0),
                    mCurrentZoom(0),
                    mSupportedPictureSizesCount(15),
                    mDumpFrmCnt(0), mDumpSkipCnt(0),
                    mPictureSizeCount(15),
                    mPreviewSizeCount(13),
                    mVideoSizeCount(0),
                    mAutoFocusRunning(false),
                    mNeedToUnlockCaf(false),
                    mHasAutoFocusSupport(false),
                    mInitialized(false),
                    mIs3DModeOn(0),
                    mSmoothZoomRunning(false),
                    mParamStringInitialized(false),
                    mFaceDetectOn(0),
                    mDisEnabled(0),
                    mZoomSupported(false),
                    mFullLiveshotEnabled(false),
                    mRecordingHint(false),
                    mAppRecordingHint(false),
                    mStatsOn(0), mCurrentHisto(-1), mSendData(false), mStatHeap(NULL),
                    mZslLookBackMode(0),
                    mZslLookBackValue(0),
                    mZslEmptyQueueFlag(false),
                    mPictureSizes(NULL),
                    mVideoSizes(NULL),
                    mCameraState(CAMERA_STATE_UNINITED),
                    mPostPreviewHeap(NULL),
                    mHdrMode(HDR_BRACKETING_OFF),
                    mStreamLiveSnap(NULL),
                    mExifTableNumEntries(0),
                    mDenoiseValue(0),
                    mSnapshotFormat(0),
                    mStartRecording(0),
                    mZslInterval(1),
                    mBrightness(0),
                    mContrast(0),
                    mEffects(0),
                    mBestShotMode(0),
                    mHJR(0),
                    mSkinToneEnhancement(0),
                    mRotation(0),
                    mFocusMode(AF_MODE_MAX),
                    mPreviewFormat(CAMERA_YUV_420_NV21),
                    mRestartPreview(false),
                    mReleasedRecordingFrame(false),
                    mStateLiveshot(false),
                    isCameraOpen(false),
                    mPauseFramedispatch(false),
                    mSnapJpegCbRunning(false),
                    mSnapCbDisabled(false)
{
    ALOGV("QCameraHardwareInterface: E");
    int32_t result = MM_CAMERA_E_GENERAL;
    char value[PROPERTY_VALUE_MAX];

    pthread_mutex_init(&mAsyncCmdMutex, NULL);
    pthread_cond_init(&mAsyncCmdWait, NULL);
    mFlashCond = false;

    property_get("persist.debug.sf.showfps", value, "0");
    mDebugFps = atoi(value);
    mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;
    mPreviewWindow = NULL;
    property_get("camera.hal.fps", value, "0");
    mFps = atoi(value);

    ALOGV("Init mPreviewState = %d", mPreviewState);

    property_get("persist.camera.hal.multitouchaf", value, "0");
    mMultiTouch = atoi(value);

    property_get("persist.camera.full.liveshot", value, "0");
    mFullLiveshotEnabled = atoi(value);

    property_get("persist.camera.hal.dis", value, "0");
    mDisEnabled = atoi(value);

    /* Open camera stack! */
    result=cam_ops_open(mCameraId, MM_CAMERA_OP_MODE_NOTUSED);
    if (result == MM_CAMERA_OK) {
      int i;
      mm_camera_event_type_t evt;
      for (i = 0; i < MM_CAMERA_EVT_TYPE_MAX; i++) {
        evt = (mm_camera_event_type_t) i;
        if (cam_evt_is_event_supported(mCameraId, evt)){
            cam_evt_register_event_notify(mCameraId,
              HAL_event_cb, (void *)this, evt);
        }
      }
    }
    ALOGV("Cam open returned %d",result);
    if(MM_CAMERA_OK != result) {
          ALOGE("startCamera: cam_ops_open failed: id = %d", mCameraId);
          return;
    }

    loadTables();
    /* Setup Picture Size and Preview size tables */
    setPictureSizeTable();
    ALOGV("%s: Picture table size: %d", __func__, mPictureSizeCount);
    ALOGV("%s: Picture table: ", __func__);
    for(unsigned int i=0; i < mPictureSizeCount;i++) {
      ALOGV(" %d  %d", mPictureSizes[i].width, mPictureSizes[i].height);
    }

    setPreviewSizeTable();
    ALOGV("%s: Preview table size: %d", __func__, mPreviewSizeCount);
    ALOGV("%s: Preview table: ", __func__);
    for(unsigned int i=0; i < mPreviewSizeCount;i++) {
      ALOGV(" %d  %d", mPreviewSizes[i].width, mPreviewSizes[i].height);
    }

    setVideoSizeTable();
    ALOGV("%s: Video table size: %d", __func__, mVideoSizeCount);
    ALOGV("%s: Video table: ", __func__);
    for(unsigned int i=0; i < mVideoSizeCount;i++) {
      ALOGV(" %d  %d", mVideoSizes[i].width, mVideoSizes[i].height);
    }

    isCameraOpen = true;
    /* set my mode - update myMode member variable due to difference in
     enum definition between upper and lower layer*/
    setMyMode(mode);
    initDefaultParameters();

    //Create Stream Objects
    //Preview
    result = createPreview();
    if(result != MM_CAMERA_OK) {
        ALOGE("%s X: Failed to create Preview Object",__func__);
        return;
    }

    //Record
    result = createRecord();
    if(result != MM_CAMERA_OK) {
        ALOGE("%s X: Failed to create Record Object",__func__);
        return;
    }

    //Snapshot
    result = createSnapshot();
    if(result != MM_CAMERA_OK) {
        ALOGE("%s X: Failed to create Record Object",__func__);
        return;
    }
    mCameraState = CAMERA_STATE_READY;
    libdnr = dlopen("libmorpho_noise_reduction.so", RTLD_NOW);
    if (libdnr) {
        ALOGV("Open MM camera DL libmorpho_noise_reduction loaded at %p & %p ", libdnr);
        *(void **)&LINK_morpho_DNR_ProcessFrame = dlsym(libdnr, "LINK_mm_camera_morpho_noise_reduction");
    }
    else
        ALOGE("failed to open libmorpho_noise_reduction");

    ALOGV("QCameraHardwareInterface: X");
}

QCameraHardwareInterface::~QCameraHardwareInterface()
{
    ALOGV("~QCameraHardwareInterface: E");
    int result;

    switch(mPreviewState) {
    case QCAMERA_HAL_PREVIEW_STOPPED:
        break;
    case QCAMERA_HAL_PREVIEW_START:
        break;
    case QCAMERA_HAL_PREVIEW_STARTED:
        stopPreview();
    break;
    case QCAMERA_HAL_RECORDING_STARTED:
        stopRecordingInternal();
        stopPreview();
        break;
    case QCAMERA_HAL_TAKE_PICTURE:
        cancelPictureInternal();
        break;
    default:
        break;
    }
    mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;

    if (isCameraOpen) {
        freePictureTable();
        freeVideoSizeTable();
        if(mStatHeap != NULL) {
          mStatHeap.clear( );
          mStatHeap = NULL;
        }
    }
    /*First stop the polling threads*/
    ALOGI("%s First stop the polling threads before deleting instances", __func__);
    cam_ops_stop(mCameraId);
    /* Join the threads, complete operations and then delete
       the instances. */
    if(mStreamDisplay){
        QCameraStream_preview::deleteInstance (mStreamDisplay);
        mStreamDisplay = NULL;
    }
    if(mStreamRecord) {
        QCameraStream_record::deleteInstance (mStreamRecord);
        mStreamRecord = NULL;
    }
    if(mStreamSnap) {
        QCameraStream_Snapshot::deleteInstance (mStreamSnap);
        mStreamSnap = NULL;
    }
    if (libdnr != NULL) {
         dlclose(libdnr);
         libdnr = NULL;
         ALOGV("closed libmorpho_noise_reduction.so");
    }

    if (mStreamLiveSnap){
        QCameraStream_Snapshot::deleteInstance (mStreamLiveSnap);
        mStreamLiveSnap = NULL;
    }

    /* Now close the camera after deleting all the instances */
    cam_ops_close(mCameraId);
    pthread_mutex_destroy(&mAsyncCmdMutex);
    pthread_cond_destroy(&mAsyncCmdWait);
    isCameraOpen = false;

    ALOGV("~QCameraHardwareInterface: X");
}

bool QCameraHardwareInterface::isCameraReady()
{
    ALOGV("isCameraReady mCameraState %d", mCameraState);
    return (mCameraState == CAMERA_STATE_READY);
}

void QCameraHardwareInterface::release()
{
    ALOGV("release: E");
    Mutex::Autolock l(&mLock);

    switch(mPreviewState) {
    case QCAMERA_HAL_PREVIEW_STOPPED:
        break;
    case QCAMERA_HAL_PREVIEW_START:
        break;
    case QCAMERA_HAL_PREVIEW_STARTED:
        stopPreviewInternal();
    break;
    case QCAMERA_HAL_RECORDING_STARTED:
        stopRecordingInternal();
        stopPreviewInternal();
        break;
    case QCAMERA_HAL_TAKE_PICTURE:
        cancelPictureInternal();
        break;
    default:
        break;
    }
#if 0
    if (isRecordingRunning()) {
        stopRecordingInternal();
        ALOGI("release: stopRecordingInternal done.");
    }
    if (isPreviewRunning()) {
        stopPreview(); //stopPreviewInternal();
        ALOGI("release: stopPreviewInternal done.");
    }
    if (isSnapshotRunning()) {
        cancelPictureInternal();
        ALOGI("release: cancelPictureInternal done.");
    }
    if (mCameraState == CAMERA_STATE_ERROR) {
        //TBD: If Error occurs then tear down
        ALOGI("release: Tear down.");
    }
#endif
    mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;
    ALOGV("release: X");
}

void QCameraHardwareInterface::setCallbacks(
    camera_notify_callback notify_cb,
    camera_data_callback data_cb,
    camera_data_timestamp_callback data_cb_timestamp,
    camera_request_memory get_memory,
    void *user)
{
    ALOGV("setCallbacks: E");
    Mutex::Autolock lock(mLock);
    mNotifyCb        = notify_cb;
    mDataCb          = data_cb;
    mDataCbTimestamp = data_cb_timestamp;
    mGetMemory       = get_memory;
    mCallbackCookie  = user;
    ALOGV("setCallbacks: X");
}

void QCameraHardwareInterface::enableMsgType(int32_t msgType)
{
    ALOGV("enableMsgType: E, msgType =0x%x", msgType);
    Mutex::Autolock lock(mLock);
    mMsgEnabled |= msgType;
    ALOGV("enableMsgType: X, msgType =0x%x, mMsgEnabled=0x%x", msgType, mMsgEnabled);
}

void QCameraHardwareInterface::disableMsgType(int32_t msgType)
{
    ALOGV("disableMsgType: E");
    Mutex::Autolock lock(mLock);
    mMsgEnabled &= ~msgType;
    ALOGV("disableMsgType: X, msgType =0x%x, mMsgEnabled=0x%x", msgType, mMsgEnabled);
}

int QCameraHardwareInterface::msgTypeEnabled(int32_t msgType)
{
    ALOGV("msgTypeEnabled: E");
    Mutex::Autolock lock(mLock);
    return (mMsgEnabled & msgType);
    ALOGV("msgTypeEnabled: X");
}
#if 0
status_t QCameraHardwareInterface::dump(int fd, const Vector<String16>& args) const
{
    ALOGI("dump: E");
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    AutoMutex lock(&mLock);
    write(fd, result.string(), result.size());
    ALOGI("dump: E");
    return NO_ERROR;
}
#endif

int QCameraHardwareInterface::dump(int fd)
{
    ALOGE("%s: not supported yet", __func__);
    return -1;
}

status_t QCameraHardwareInterface::sendCommand(int32_t command, int32_t arg1,
                                         int32_t arg2)
{
    ALOGV("sendCommand: E");
    status_t rc = NO_ERROR;
    Mutex::Autolock l(&mLock);

    switch (command) {
        case CAMERA_CMD_START_FACE_DETECTION:
           if(supportsFaceDetection() == false){
                ALOGE("Face detection support is not available");
                return NO_ERROR;
           }
           setFaceDetection("on");
           return runFaceDetection();
        case CAMERA_CMD_STOP_FACE_DETECTION:
           if(supportsFaceDetection() == false){
                ALOGE("Face detection support is not available");
                return NO_ERROR;
           }
           setFaceDetection("off");
           return runFaceDetection();

    #if 0
        case CAMERA_CMD_HISTOGRAM_ON:
            ALOGE("histogram set to on");
            rc = setHistogram(1);
            break;
        case CAMERA_CMD_HISTOGRAM_OFF:
            ALOGE("histogram set to off");
            rc = setHistogram(0);
            break;
        case CAMERA_CMD_HISTOGRAM_SEND_DATA:
            ALOGE("histogram send data");
            mSendData = true;
            rc = NO_ERROR;
            break;
       case CAMERA_CMD_SEND_META_DATA:
           mMetaDataWaitLock.lock();
           if(mFaceDetectOn == true) {
               mSendMetaData = true;
           }
           mMetaDataWaitLock.unlock();
           return NO_ERROR;
#endif
#if 0 /* To Do: will enable it later */
        case CAMERA_CMD_START_SMOOTH_ZOOM :
            ALOGV("HAL sendcmd start smooth zoom %d %d", arg1 , arg2);
            /*TO DO: get MaxZoom from parameter*/
            int MaxZoom = 100;

            switch(mCameraState ) {
                case CAMERA_STATE_PREVIEW:
                case CAMERA_STATE_RECORD_CMD_SENT:
                case CAMERA_STATE_RECORD:
                    mTargetSmoothZoom = arg1;
                    mCurrentZoom = mParameters.getInt("zoom");
                    mSmoothZoomStep = (mCurrentZoom > mTargetSmoothZoom)? -1: 1;
                   if(mCurrentZoom == mTargetSmoothZoom) {
                        ALOGV("Smoothzoom target zoom value is same as "
                        "current zoom value, return...");
                        mNotifyCallback(CAMERA_MSG_ZOOM,
                        mCurrentZoom, 1, mCallbackCookie);
                    } else if(mCurrentZoom < 0 || mCurrentZoom > MaxZoom ||
                        mTargetSmoothZoom < 0 || mTargetSmoothZoom > MaxZoom)  {
                        ALOGE(" ERROR : beyond supported zoom values, break..");
                        mNotifyCallback(CAMERA_MSG_ZOOM,
                        mCurrentZoom, 0, mCallbackCookie);
                    } else {
                        mSmoothZoomRunning = true;
                        mCurrentZoom += mSmoothZoomStep;
                        if ((mSmoothZoomStep < 0 && mCurrentZoom < mTargetSmoothZoom)||
                        (mSmoothZoomStep > 0 && mCurrentZoom > mTargetSmoothZoom )) {
                            mCurrentZoom = mTargetSmoothZoom;
                        }
                        mParameters.set("zoom", mCurrentZoom);
                        setZoom(mParameters);
                    }
                    break;
                default:
                    ALOGV(" No preview, no smoothzoom ");
                    break;
            }
            rc = NO_ERROR;
            break;

        case CAMERA_CMD_STOP_SMOOTH_ZOOM:
            if(mSmoothZoomRunning) {
                mSmoothZoomRunning = false;
                /*To Do: send cmd to stop zooming*/
            }
            ALOGV("HAL sendcmd stop smooth zoom");
            rc = NO_ERROR;
            break;
#endif
        default:
            break;
    }
    ALOGV("sendCommand: X");
    return rc;
}

void QCameraHardwareInterface::setMyMode(int mode)
{
    ALOGV("setMyMode: E");
    //if (mode & CAMERA_SUPPORT_MODE_3D) {
    //    myMode = CAMERA_MODE_3D;
    //}else {
        /* default mode is 2D */
        myMode = CAMERA_MODE_2D;
    //}

    //if (mode & CAMERA_SUPPORT_MODE_ZSL) {
    //    myMode = (camera_mode_t)(myMode |CAMERA_ZSL_MODE);
    //}else {
       myMode = (camera_mode_t) (myMode | CAMERA_NONZSL_MODE);
    //}
    ALOGV("setMyMode: Set mode to %d (passed mode: %d)", myMode, mode);
    ALOGV("setMyMode: X");
}
/* static factory function */
QCameraHardwareInterface *QCameraHardwareInterface::createInstance(int cameraId, int mode)
{
    ALOGV("createInstance: E");
    QCameraHardwareInterface *cam = new QCameraHardwareInterface(cameraId, mode);
    if (cam ) {
      if (cam->mCameraState != CAMERA_STATE_READY) {
        ALOGE("createInstance: Failed");
        delete cam;
        cam = NULL;
      }
    }

    if (cam) {
      //sp<CameraHardwareInterface> hardware(cam);
      ALOGV("createInstance: X");
      return cam;
    } else {
      return NULL;
    }
}
/* external plug in function */
extern "C" void *
QCameraHAL_openCameraHardware(int  cameraId, int mode)
{
    ALOGV("QCameraHAL_openCameraHardware: E");
    return (void *) QCameraHardwareInterface::createInstance(cameraId, mode);
}

bool QCameraHardwareInterface::isPreviewRunning() {
    ALOGV("isPreviewRunning: E");
    bool ret = false;
    ALOGV("isPreviewRunning: camera state:%d", mCameraState);

    if((mCameraState == CAMERA_STATE_PREVIEW) ||
       (mCameraState == CAMERA_STATE_PREVIEW_START_CMD_SENT) ||
       (mCameraState == CAMERA_STATE_RECORD) ||
       (mCameraState == CAMERA_STATE_RECORD_START_CMD_SENT) ||
       (mCameraState == CAMERA_STATE_ZSL) ||
       (mCameraState == CAMERA_STATE_ZSL_START_CMD_SENT)){
       return true;
    }
    ALOGV("isPreviewRunning: X");
    return ret;
}

bool QCameraHardwareInterface::isRecordingRunning() {
    ALOGV("isRecordingRunning: E");
    bool ret = false;
    if(QCAMERA_HAL_RECORDING_STARTED == mPreviewState)
      ret = true;
    //if((mCameraState == CAMERA_STATE_RECORD) ||
    //   (mCameraState == CAMERA_STATE_RECORD_START_CMD_SENT)) {
    //   return true;
    //}
    ALOGV("isRecordingRunning: X");
    return ret;
}

bool QCameraHardwareInterface::isSnapshotRunning() {
    ALOGV("isSnapshotRunning: E");
    bool ret = false;
    //if((mCameraState == CAMERA_STATE_SNAP_CMD_ACKED) ||
    //   (mCameraState == CAMERA_STATE_SNAP_START_CMD_SENT)) {
    //    return true;
    //}
    switch(mPreviewState) {
    case QCAMERA_HAL_PREVIEW_STOPPED:
    case QCAMERA_HAL_PREVIEW_START:
    case QCAMERA_HAL_PREVIEW_STARTED:
    case QCAMERA_HAL_RECORDING_STARTED:
    default:
        break;
    case QCAMERA_HAL_TAKE_PICTURE:
        ret = true;
        break;
    }
    ALOGV("isSnapshotRunning: X");
    return ret;
}

bool QCameraHardwareInterface::isZSLMode() {
    return (myMode & CAMERA_ZSL_MODE);
}

int QCameraHardwareInterface::getHDRMode() {
    return mHdrMode;
}

void QCameraHardwareInterface::debugShowPreviewFPS() const
{
    static int mFrameCount;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    nsecs_t now = systemTime();
    nsecs_t diff = now - mLastFpsTime;
    if (diff > ms2ns(250)) {
        mFps =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        ALOGV("Preview Frames Per Second: %.4f", mFps);
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
    }
}

void QCameraHardwareInterface::
processPreviewChannelEvent(mm_camera_ch_event_type_t channelEvent, app_notify_cb_t *app_cb) {
    ALOGV("processPreviewChannelEvent: E");
    switch(channelEvent) {
        case MM_CAMERA_CH_EVT_STREAMING_ON:
            mCameraState =
                isZSLMode() ? CAMERA_STATE_ZSL : CAMERA_STATE_PREVIEW;
            break;
        case MM_CAMERA_CH_EVT_STREAMING_OFF:
            mCameraState = CAMERA_STATE_READY;
            break;
        case MM_CAMERA_CH_EVT_DATA_DELIVERY_DONE:
            break;
        default:
            break;
    }
    ALOGV("processPreviewChannelEvent: X");
    return;
}

void QCameraHardwareInterface::processRecordChannelEvent(
  mm_camera_ch_event_type_t channelEvent, app_notify_cb_t *app_cb) {
    ALOGV("processRecordChannelEvent: E");
    switch(channelEvent) {
        case MM_CAMERA_CH_EVT_STREAMING_ON:
            mCameraState = CAMERA_STATE_RECORD;
            break;
        case MM_CAMERA_CH_EVT_STREAMING_OFF:
            mCameraState = CAMERA_STATE_PREVIEW;
            break;
        case MM_CAMERA_CH_EVT_DATA_DELIVERY_DONE:
            break;
        default:
            break;
    }
    ALOGV("processRecordChannelEvent: X");
    return;
}

void QCameraHardwareInterface::
processSnapshotChannelEvent(mm_camera_ch_event_type_t channelEvent, app_notify_cb_t *app_cb) {
    ALOGV("processSnapshotChannelEvent: E evt=%d state=%d", channelEvent,
      mCameraState);
    switch(channelEvent) {
        case MM_CAMERA_CH_EVT_STREAMING_ON:
          if (!mStateLiveshot) {
            mCameraState =
              isZSLMode() ? CAMERA_STATE_ZSL : CAMERA_STATE_SNAP_CMD_ACKED;
          }
          break;
        case MM_CAMERA_CH_EVT_STREAMING_OFF:
          if (!mStateLiveshot) {
            mCameraState = CAMERA_STATE_READY;
          }
          break;
        case MM_CAMERA_CH_EVT_DATA_DELIVERY_DONE:
            break;
        case MM_CAMERA_CH_EVT_DATA_REQUEST_MORE:
            if (isZSLMode()) {
                /* ZSL Mode: In ZSL Burst Mode, users may request for number of
                snapshots larger than internal size of ZSL queue. So we'll need
                process the remaining frames as they become available.
                In such case, we'll get this event */
                if(NULL != mStreamSnap)
                  mStreamSnap->takePictureZSL();
            }
            break;
        default:
            break;
    }
    ALOGV("processSnapshotChannelEvent: X");
    return;
}

void QCameraHardwareInterface::processChannelEvent(
  mm_camera_ch_event_t *event, app_notify_cb_t *app_cb)
{
    ALOGV("processChannelEvent: E");
    Mutex::Autolock lock(mLock);
    switch(event->ch) {
        case MM_CAMERA_CH_PREVIEW:
            processPreviewChannelEvent(event->evt, app_cb);
            break;
        case MM_CAMERA_CH_VIDEO:
            processRecordChannelEvent(event->evt, app_cb);
            break;
        case MM_CAMERA_CH_SNAPSHOT:
            processSnapshotChannelEvent(event->evt, app_cb);
            break;
        default:
            break;
    }
    ALOGV("processChannelEvent: X");
    return;
}

void QCameraHardwareInterface::processCtrlEvent(mm_camera_ctrl_event_t *event, app_notify_cb_t *app_cb)
{
    ALOGV("processCtrlEvent: %d, E",event->evt);
    Mutex::Autolock lock(mLock);
    switch(event->evt)
    {
        case MM_CAMERA_CTRL_EVT_ZOOM_DONE:
            zoomEvent(&event->status, app_cb);
            break;
        case MM_CAMERA_CTRL_EVT_AUTO_FOCUS_DONE:
            autoFocusEvent(&event->status, app_cb);
            break;
        case MM_CAMERA_CTRL_EVT_AUTO_FOCUS_MOVE:
            autoFocusMoveEvent(&event->status, app_cb);
            break;
        case MM_CAMERA_CTRL_EVT_PREP_SNAPSHOT:
            break;
        case MM_CAMERA_CTRL_EVT_WDN_DONE:
            wdenoiseEvent(event->status, (void *)(event->cookie));
            break;
        case MM_CAMERA_CTRL_EVT_HDR_DONE:
            hdrEvent(event->status, (void *)(event->cookie));
            break;
        case MM_CAMERA_CTRL_EVT_ERROR:
            app_cb->notifyCb  = mNotifyCb;
            app_cb->argm_notify.msg_type = CAMERA_MSG_ERROR;
            app_cb->argm_notify.ext1 = CAMERA_ERROR_UNKNOWN;
            app_cb->argm_notify.cookie =  mCallbackCookie;
            break;
      case MM_CAMERA_CTRL_EVT_SNAPSHOT_CONFIG_DONE:
          ALOGV("%s: MM_CAMERA_CTRL_EVT_SNAPSHOT_CONFIG_DONE", __func__);
          app_cb->notifyCb  = mNotifyCb;
          app_cb->argm_notify.msg_type = CAMERA_MSG_SHUTTER;
          app_cb->argm_notify.ext1 = 0;
          app_cb->argm_notify.ext2 = true;
          app_cb->argm_notify.cookie =  mCallbackCookie;
          mShutterSoundPlayed = true;
       default:
            break;
    }
    ALOGV("processCtrlEvent: X");
    return;
}

void  QCameraHardwareInterface::processStatsEvent(
  mm_camera_stats_event_t *event, app_notify_cb_t *app_cb)
{
    ALOGV("processStatsEvent: E");
    if (!isPreviewRunning( )) {
        ALOGE("preview is not running");
        return;
    }

    switch (event->event_id) {

        case MM_CAMERA_STATS_EVT_HISTO:
        {
        #if 0
            ALOGE("HAL process Histo: mMsgEnabled=0x%x, mStatsOn=%d, mSendData=%d, mDataCb=%p ",
            (mMsgEnabled & CAMERA_MSG_STATS_DATA), mStatsOn, mSendData, mDataCb);
            int msgEnabled = mMsgEnabled;
            /*get stats buffer based on index*/
            camera_preview_histogram_info* hist_info =
                (camera_preview_histogram_info*) mHistServer.camera_memory[event->e.stats_histo.index]->data;

            if(mStatsOn == QCAMERA_PARM_ENABLE && mSendData &&
                            mDataCb && (msgEnabled & CAMERA_MSG_STATS_DATA) ) {
                uint32_t *dest;
                mSendData = false;
                mCurrentHisto = (mCurrentHisto + 1) % 3;
                // The first element of the array will contain the maximum hist value provided by driver.
                *(uint32_t *)((unsigned int)(mStatsMapped[mCurrentHisto]->data)) = hist_info->max_value;
                memcpy((uint32_t *)((unsigned int)mStatsMapped[mCurrentHisto]->data + sizeof(int32_t)),
                                                    (uint32_t *)hist_info->buffer,(sizeof(int32_t) * 256));

                app_cb->dataCb  = mDataCb;
                app_cb->argm_data_cb.msg_type = CAMERA_MSG_STATS_DATA;
                app_cb->argm_data_cb.data = mStatsMapped[mCurrentHisto];
                app_cb->argm_data_cb.index = 0;
                app_cb->argm_data_cb.metadata = NULL;
                app_cb->argm_data_cb.cookie =  mCallbackCookie;
            }
        #endif
            break;

        }
        default:
        break;
    }
  ALOGV("receiveCameraStats X");
}

void  QCameraHardwareInterface::processInfoEvent(
  mm_camera_info_event_t *event, app_notify_cb_t *app_cb) {
    ALOGV("processInfoEvent: %d, E",event->event_id);
    //Mutex::Autolock lock(eventLock);
    switch(event->event_id)
    {
        case MM_CAMERA_INFO_EVT_ROI:
            roiEvent(event->e.roi, app_cb);
            break;
        default:
            break;
    }
    ALOGV("processInfoEvent: X");
    return;
}

void  QCameraHardwareInterface::processEvent(mm_camera_event_t *event)
{
    app_notify_cb_t app_cb;
    ALOGV("processEvent: type :%d E",event->event_type);
    if(mPreviewState == QCAMERA_HAL_PREVIEW_STOPPED){
	ALOGV("Stop recording issued. Return from process Event");
        return;
    }
    memset(&app_cb, 0, sizeof(app_notify_cb_t));
    switch(event->event_type)
    {
        case MM_CAMERA_EVT_TYPE_CH:
            processChannelEvent(&event->e.ch, &app_cb);
            break;
        case MM_CAMERA_EVT_TYPE_CTRL:
            processCtrlEvent(&event->e.ctrl, &app_cb);
            break;
        case MM_CAMERA_EVT_TYPE_STATS:
            processStatsEvent(&event->e.stats, &app_cb);
            break;
        case MM_CAMERA_EVT_TYPE_INFO:
            processInfoEvent(&event->e.info, &app_cb);
            break;
        default:
            break;
    }
    ALOGV(" App_cb Notify %p, datacb=%p", app_cb.notifyCb, app_cb.dataCb);
    if (app_cb.notifyCb) {
      app_cb.notifyCb(app_cb.argm_notify.msg_type,
        app_cb.argm_notify.ext1, app_cb.argm_notify.ext2,
        app_cb.argm_notify.cookie);
    }
    if (app_cb.dataCb) {
      app_cb.dataCb(app_cb.argm_data_cb.msg_type,
        app_cb.argm_data_cb.data, app_cb.argm_data_cb.index,
        app_cb.argm_data_cb.metadata, app_cb.argm_data_cb.cookie);
    }
    ALOGV("processEvent: X");
    return;
}

bool QCameraHardwareInterface::preview_parm_config (cam_ctrl_dimension_t* dim,
                                   QCameraParameters& parm)
{
    ALOGV("preview_parm_config: E");
    bool matching = true;
    int display_width = 0;  /* width of display      */
    int display_height = 0; /* height of display */
    uint16_t video_width = 0;  /* width of the video  */
    uint16_t video_height = 0; /* height of the video */

    /* First check if the preview resolution is the same, if not, change it*/
    parm.getPreviewSize(&display_width,  &display_height);
    if (display_width && display_height) {
        matching = (display_width == dim->display_width) &&
            (display_height == dim->display_height);

        if (!matching) {
            dim->display_width  = display_width;
            dim->display_height = display_height;
        }
    }
    else
        matching = false;

    cam_format_t value = getPreviewFormat();

    if(value != NOT_FOUND && value != dim->prev_format ) {
        //Setting to Parameter requested by the Upper layer
        dim->prev_format = value;
    } else if (value == NOT_FOUND) {
        //Setting to default Format.
        dim->prev_format = CAMERA_YUV_420_NV21;
    }
    mPreviewFormat = dim->prev_format;

    dim->prev_padding_format =  getPreviewPadding( );

    dim->enc_format = CAMERA_YUV_420_NV12;
    dim->orig_video_width = mDimension.orig_video_width;
    dim->orig_video_height = mDimension.orig_video_height;
    dim->video_width = mDimension.video_width;
    dim->video_height = mDimension.video_height;
    dim->video_chroma_width = mDimension.video_width;
    dim->video_chroma_height  = mDimension.video_height;
    /* Reset the Main image and thumbnail formats here,
     * since they might have been changed when video size
     * livesnapshot was taken. */
    if (mSnapshotFormat == 1)
      dim->main_img_format = CAMERA_YUV_422_NV61;
    else
      dim->main_img_format = CAMERA_YUV_420_NV21;
    dim->thumb_format = CAMERA_YUV_420_NV21;
    ALOGV("preview_parm_config: X");
    return matching;
}

status_t QCameraHardwareInterface::startPreview()
{
    status_t retVal = NO_ERROR;

    ALOGV("%s: mPreviewState =%d", __func__, mPreviewState);
    Mutex::Autolock lock(mLock);

    if(mPauseFramedispatch){
        //In ZSL case after snapshot we need to restart
        //if stop preview not issued.
        stopPreviewInternal();
        mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;
        mPauseFramedispatch = false;
    }

    switch(mPreviewState) {
    case QCAMERA_HAL_PREVIEW_STOPPED:
    case QCAMERA_HAL_TAKE_PICTURE:
        mPreviewState = QCAMERA_HAL_PREVIEW_START;
        ALOGV("%s:  HAL::startPreview begin", __func__);

        if(QCAMERA_HAL_PREVIEW_START == mPreviewState &&
           (mPreviewWindow)) {
            ALOGD("%s:  start preview now", __func__);
            retVal = startPreview2();
            if(retVal == NO_ERROR)
                mPreviewState = QCAMERA_HAL_PREVIEW_STARTED;
        } else {
            ALOGV("%s:  received startPreview, but preview window = null", __func__);
        }
        break;
    case QCAMERA_HAL_PREVIEW_START:
    case QCAMERA_HAL_PREVIEW_STARTED:
    break;
    case QCAMERA_HAL_RECORDING_STARTED:
        ALOGV("%s: cannot start preview in recording state", __func__);
        break;
    default:
        ALOGE("%s: unknow state %d received", __func__, mPreviewState);
        retVal = UNKNOWN_ERROR;
        break;
    }
    return retVal;
}

status_t QCameraHardwareInterface::startPreview2()
{
    ALOGV("startPreview2: E");
    status_t ret = NO_ERROR;

    cam_ctrl_dimension_t dim;
    mm_camera_dimension_t maxDim;
    bool initPreview = false;

    mPauseFramedispatch = false;
    if (mPreviewState == QCAMERA_HAL_PREVIEW_STARTED) { //isPreviewRunning()){
        ALOGE("%s:Preview already started  mCameraState = %d!", __func__, mCameraState);
        ALOGV("%s: X", __func__);
        return NO_ERROR;
    }

    const char *str = mParameters.get(QCameraParameters::KEY_SCENE_MODE);

    if (mRecordingHint || mFlashCond || !strcmp(str, "hdr")) {
        ALOGI("%s:Setting non-ZSL mode",__func__);
        mParameters.set(QCameraParameters::KEY_CAMERA_MODE, 0);
        myMode = (camera_mode_t)(myMode & ~CAMERA_ZSL_MODE);
        mParameters.setPreviewFrameRateMode("frame-rate-auto");
        setPreviewFrameRateMode(mParameters);
     } else {
        ALOGI("%s:Setting ZSL mode",__func__);
        mParameters.set(QCameraParameters::KEY_CAMERA_MODE, 1);
        myMode = (camera_mode_t)(myMode | CAMERA_ZSL_MODE);
        mParameters.setPreviewFrameRateMode("frame-rate-auto");
        setPreviewFrameRateMode(mParameters);

    }

    /*  get existing preview information, by qury mm_camera*/
    memset(&dim, 0, sizeof(cam_ctrl_dimension_t));
    ret = cam_config_get_parm(mCameraId, MM_CAMERA_PARM_DIMENSION,&dim);

    if (MM_CAMERA_OK != ret) {
      ALOGE("%s: error - can't get preview dimension!", __func__);
      ALOGV("%s: X", __func__);
      return BAD_VALUE;
    }

    /* config the parmeters and see if we need to re-init the stream*/
    initPreview = preview_parm_config (&dim, mParameters);

    if (mRecordingHint && mFullLiveshotEnabled) {
#if 0
      /* Camcorder mode and Full resolution liveshot enabled
       * TBD lookup table for correct aspect ratio matching size */
      memset(&maxDim, 0, sizeof(mm_camera_dimension_t));
      getMaxPictureDimension(&maxDim);
      if (!maxDim.width || !maxDim.height) {
        maxDim.width  = DEFAULT_LIVESHOT_WIDTH;
        maxDim.height = DEFAULT_LIVESHOT_HEIGHT;
      }
      /* TODO Remove this hack after adding code to get live shot dimension */
      if (!mCameraId) {
        maxDim.width = DEFAULT_LIVESHOT_WIDTH;
        maxDim.height = DEFAULT_LIVESHOT_HEIGHT;
      }
      dim.picture_width = maxDim.width;
      dim.picture_height = maxDim.height;
      mParameters.setPictureSize(dim.picture_width, dim.picture_height);
      ALOGI("%s Setting Liveshot dimension as %d x %d", __func__,
           maxDim.width, maxDim.height);
#endif
        int mPictureWidth, mPictureHeight;
        bool matching;
        /* First check if the picture resolution is the same, if not, change it*/
        getPictureSize(&mPictureWidth, &mPictureHeight);

        matching = (mPictureWidth == dim.picture_width) &&
            (mPictureHeight == dim.picture_height);

        if (!matching) {
            dim.picture_width  = mPictureWidth;
            dim.picture_height = mPictureHeight;
            //For FullSize snapshot Main image Buffer is used as Thumanail.
            dim.ui_thumbnail_height = mPictureWidth;
            dim.ui_thumbnail_width = mPictureHeight;
        }
        ALOGI("%s: Fullsize Liveshaot Picture size to set: %d x %d", __func__,
             dim.picture_width, dim.picture_height);
        mParameters.setPictureSize(dim.picture_width, dim.picture_height);
    }

    ret = cam_config_set_parm(mCameraId, MM_CAMERA_PARM_DIMENSION,&dim);
    if (MM_CAMERA_OK != ret) {
      ALOGE("%s X: error - can't config preview parms!", __func__);
      return BAD_VALUE;
    }

    mStreamDisplay->setMode(myMode & CAMERA_ZSL_MODE);
    mStreamSnap->setMode(myMode & CAMERA_ZSL_MODE);
    mStreamRecord->setMode(myMode & CAMERA_ZSL_MODE);
    ALOGV("%s: myMode = %d", __func__, myMode);

    ALOGV("%s: setPreviewWindow", __func__);
    mStreamDisplay->setPreviewWindow(mPreviewWindow);
    ret = cam_config_set_parm(mCameraId, MM_CAMERA_PARM_INFORM_STARTPRVIEW, NULL);
    if(ret<0)
    {
      ALOGE("%s: Failed to Check MM_CAMERA_PARM_INFORM_STARTPRVIEW, rc %d", __func__, ret);
    }

    if(isZSLMode()) {
        /* Start preview streaming */
        ret = mStreamDisplay->start();
        if (MM_CAMERA_OK != ret){
            ALOGE("%s: X -error - can't start nonZSL stream!", __func__);
            return BAD_VALUE;
        }

        /* Start ZSL stream */
        ret =  mStreamSnap->start();
        if (MM_CAMERA_OK != ret){
            ALOGE("%s: error - can't start Snapshot stream!", __func__);
            mStreamDisplay->stop();
            return BAD_VALUE;
        }
    }else{
        ret = mStreamDisplay->start();
    }

    /*call QCameraStream_noneZSL::start() */
    if (MM_CAMERA_OK != ret){
      ALOGE("%s: X error - can't start stream!", __func__);
      return BAD_VALUE;
    }
    if(MM_CAMERA_OK == ret)
        mCameraState = CAMERA_STATE_PREVIEW_START_CMD_SENT;
    else
        mCameraState = CAMERA_STATE_ERROR;

    if(mPostPreviewHeap != NULL) {
        mPostPreviewHeap.clear();
        mPostPreviewHeap = NULL;
    }

    mRestartPreview = false;

    ALOGV("startPreview: X");
    return ret;
}

void QCameraHardwareInterface::stopPreview()
{
    ALOGV("%s: stopPreview: E", __func__);
    Mutex::Autolock lock(mLock);
    mm_camera_util_profile("HAL: stopPreview(): E");
    mFaceDetectOn = false;

    // reset recording hint to the value passed from Apps
    const char * str = mParameters.get(QCameraParameters::KEY_RECORDING_HINT);
    if((str != NULL) && !strcmp(str, "true")){
        mRecordingHint = true;
    } else {
        mRecordingHint = false;
    }

    const char * str_fd = mParameters.get(QCameraParameters::KEY_FACE_DETECTION);
    if((str_fd != NULL) && !strcmp(str_fd, "on")){
        if(supportsFaceDetection() == false){
         ALOGE("Face detection support is not available");
        }
        setFaceDetection("off");
        runFaceDetection();
    }

    switch(mPreviewState) {
      case QCAMERA_HAL_PREVIEW_START:
          //mPreviewWindow = NULL;
          mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;
          break;
      case QCAMERA_HAL_PREVIEW_STARTED:
          stopPreviewInternal();
          mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;
          break;
      case QCAMERA_HAL_RECORDING_STARTED:
            stopRecordingInternal();
            stopPreviewInternal();
            mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;
            break;
    case QCAMERA_HAL_TAKE_PICTURE:
        cancelPictureInternal();
        stopPreviewInternal();
        mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;
        break;
      case QCAMERA_HAL_PREVIEW_STOPPED:
      default:
            break;
    }
    ALOGV("stopPreview: X, mPreviewState = %d", mPreviewState);
}

#if 0 //mzhu
void QCameraHardwareInterface::stopPreviewZSL()
{
    ALOGI("stopPreviewZSL: E");

    if(!mStreamDisplay || !mStreamSnap) {
        ALOGE("mStreamDisplay/mStreamSnap is null");
        return;
    }
    ALOGI("stopPreview: X, mPreviewState = %d", mPreviewState);
}
#endif
void QCameraHardwareInterface::stopPreviewInternal()
{
    ALOGV("stopPreviewInternal: E");
    status_t ret = NO_ERROR;

    if(!mStreamDisplay) {
        ALOGE("mStreamDisplay is null");
        return;
    }

    if(isZSLMode()) {
        /* take care snapshot object for ZSL mode */
        mStreamSnap->stop();
    }
    mStreamDisplay->stop();

    mPauseFramedispatch = false;
    mCameraState = CAMERA_STATE_PREVIEW_STOP_CMD_SENT;
    ALOGV("stopPreviewInternal: X");
}

int QCameraHardwareInterface::previewEnabled()
{
    ALOGV("previewEnabled: E");
    Mutex::Autolock lock(mLock);
    ALOGV("%s: mCameraState = %d", __func__, mCameraState);
    if (mPauseFramedispatch) {
        return false;
    }
    switch(mPreviewState) {
    case QCAMERA_HAL_PREVIEW_STOPPED:
    case QCAMERA_HAL_TAKE_PICTURE:
    default:
        return false;
        break;
    case QCAMERA_HAL_PREVIEW_START:
    case QCAMERA_HAL_PREVIEW_STARTED:
    case QCAMERA_HAL_RECORDING_STARTED:
        return true;
        break;
    }
    return false;
}

status_t QCameraHardwareInterface::startRecording()
{
    ALOGV("startRecording: E");
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mLock);

    switch(mPreviewState) {
    case QCAMERA_HAL_PREVIEW_STOPPED:
        ALOGE("%s: preview has not been started", __func__);
        ret = UNKNOWN_ERROR;
        break;
    case QCAMERA_HAL_PREVIEW_START:
        ALOGE("%s: no preview native window", __func__);
        ret = UNKNOWN_ERROR;
        break;
    case QCAMERA_HAL_PREVIEW_STARTED:
        //remember recordinghint value set by app
        mAppRecordingHint = mRecordingHint;
        pausePreviewForVideo();
        ret =  mStreamRecord->start();
        if (MM_CAMERA_OK != ret){
            ALOGE("%s: error - mStreamRecord->start!", __func__);
            ret = BAD_VALUE;
            break;
        }
        if(MM_CAMERA_OK == ret)
            mCameraState = CAMERA_STATE_RECORD_START_CMD_SENT;
        else
            mCameraState = CAMERA_STATE_ERROR;
        mPreviewState = QCAMERA_HAL_RECORDING_STARTED;
        break;
    case QCAMERA_HAL_RECORDING_STARTED:
        ALOGV("%s: ", __func__);
        break;
    case QCAMERA_HAL_TAKE_PICTURE:
    default:
       ret = BAD_VALUE;
       break;
    }
    ALOGV("startRecording: X");
    return ret;
}

void QCameraHardwareInterface::stopRecording()
{
    ALOGV("stopRecording: E");
    Mutex::Autolock lock(mLock);
    switch(mPreviewState) {
    case QCAMERA_HAL_PREVIEW_STOPPED:
    case QCAMERA_HAL_PREVIEW_START:
    case QCAMERA_HAL_PREVIEW_STARTED:
        break;
    case QCAMERA_HAL_RECORDING_STARTED:
        mRecordingHint = mAppRecordingHint;
        stopRecordingInternal();
        mPreviewState = QCAMERA_HAL_PREVIEW_STARTED;
        break;
    case QCAMERA_HAL_TAKE_PICTURE:
    default:
        break;
    }
    ALOGV("stopRecording: X");

}
void QCameraHardwareInterface::stopRecordingInternal()
{
    ALOGV("stopRecordingInternal: E");
    status_t ret = NO_ERROR;

    if(!mStreamRecord) {
        ALOGE("mStreamRecord is null");
        return;
    }

    if(mStateLiveshot && mStreamLiveSnap != NULL) {
        mStreamLiveSnap->stop();
        mStateLiveshot = false;
    }
    /*
    * call QCameraStream_record::stop()
    * Unregister Callback, action stop
    */
    mStreamRecord->stop();
    mCameraState = CAMERA_STATE_PREVIEW;  //TODO : Apurva : Hacked for 2nd time Recording
    mPreviewState = QCAMERA_HAL_PREVIEW_STARTED;
    ALOGV("stopRecordingInternal: X");
    return;
}

int QCameraHardwareInterface::recordingEnabled()
{
    int ret = 0;
    Mutex::Autolock lock(mLock);
    ALOGV("%s: E", __func__);
    switch(mPreviewState) {
    case QCAMERA_HAL_PREVIEW_STOPPED:
    case QCAMERA_HAL_PREVIEW_START:
    case QCAMERA_HAL_PREVIEW_STARTED:
        break;
    case QCAMERA_HAL_RECORDING_STARTED:
        ret = 1;
        break;
    case QCAMERA_HAL_TAKE_PICTURE:
    default:
        break;
    }
    ALOGV("%s: X, ret = %d", __func__, ret);
    return ret;   //isRecordingRunning();
}

/**
* Release a record frame previously returned by CAMERA_MSG_VIDEO_FRAME.
*/
void QCameraHardwareInterface::releaseRecordingFrame(const void *opaque)
{
    ALOGV("%s : BEGIN",__func__);
    if(mStreamRecord == NULL) {
        ALOGE("Record stream Not Initialized");
        return;
    }
    mStreamRecord->releaseRecordingFrame(opaque);
    ALOGV("%s : END",__func__);
    return;
}

status_t QCameraHardwareInterface::autoFocusMoveEvent(cam_ctrl_status_t *status, app_notify_cb_t *app_cb)
{
    ALOGV("autoFocusMoveEvent: E");
    int ret = NO_ERROR;

    isp3a_af_mode_t afMode = getAutoFocusMode(mParameters);

    if (afMode == AF_MODE_CAF && mNotifyCb && ( mMsgEnabled & CAMERA_MSG_FOCUS_MOVE)){
        ALOGV("%s:Issuing AF Move callback to service",__func__);

        app_cb->notifyCb  = mNotifyCb;
        app_cb->argm_notify.msg_type = CAMERA_MSG_FOCUS_MOVE;
        app_cb->argm_notify.ext2 = 0;
        app_cb->argm_notify.cookie =  mCallbackCookie;

        ALOGV("Auto focus state =%d", *status);
        if(*status == 1) {
            app_cb->argm_notify.ext1 = true;
        }else if(*status == 0){
            app_cb->argm_notify.ext1 = false;
        }else{
            app_cb->notifyCb  = NULL;
            ALOGE("%s:Unknown AF Move Status received (%d) received",__func__,*status);
        }
    }
    ALOGV("autoFocusMoveEvent: X");
    return ret;
}

status_t QCameraHardwareInterface::autoFocusEvent(cam_ctrl_status_t *status, app_notify_cb_t *app_cb)
{
    ALOGV("autoFocusEvent: E");
    int ret = NO_ERROR;
/************************************************************
  BEGIN MUTEX CODE
*************************************************************/

    ALOGV("%s:%d: Trying to acquire AF bit lock",__func__,__LINE__);
    mAutofocusLock.lock();
    ALOGV("%s:%d: Acquired AF bit lock",__func__,__LINE__);

    if(mAutoFocusRunning==false) {
      ALOGV("%s:AF not running, discarding stale event",__func__);
      mAutofocusLock.unlock();
      return ret;
    }
    /* If autofocus call has been made during CAF, CAF will be locked.
     * We specifically need to call cancelAutoFocus to unlock CAF.
     * In that sense, AF is still running.*/
    isp3a_af_mode_t afMode = getAutoFocusMode(mParameters);
    if (afMode == AF_MODE_CAF)
        mNeedToUnlockCaf = true;
    mAutoFocusRunning = false;
    mAutofocusLock.unlock();

/************************************************************
  END MUTEX CODE
*************************************************************/
    if(status==NULL) {
      ALOGE("%s:NULL ptr received for status",__func__);
      return BAD_VALUE;
    }

    /* update focus distances after autofocus is done */
    if((*status != CAM_CTRL_FAILED) && updateFocusDistances() != NO_ERROR) {
       ALOGE("%s: updateFocusDistances failed for %d", __FUNCTION__, mFocusMode);
    }

    /*(Do?) we need to make sure that the call back is the
      last possible step in the execution flow since the same
      context might be used if a fail triggers another round
      of AF then the mAutoFocusRunning flag and other state
      variables' validity will be under question*/

    if (mNotifyCb && ( mMsgEnabled & CAMERA_MSG_FOCUS)){
      ALOGV("%s:Issuing AF callback to service",__func__);

      /* "Accepted" status is not appropriate it should be used for
        initial cmd, event reporting should only give use SUCCESS/FAIL
        */

      app_cb->notifyCb  = mNotifyCb;
      app_cb->argm_notify.msg_type = CAMERA_MSG_FOCUS;
      app_cb->argm_notify.ext2 = 0;
      app_cb->argm_notify.cookie =  mCallbackCookie;

      ALOGV("Auto focus state =%d", *status);
      if(*status==CAM_CTRL_SUCCESS) {
        app_cb->argm_notify.ext1 = true;
      }
      else if(*status==CAM_CTRL_FAILED){
        app_cb->argm_notify.ext1 = false;
      }
      else{
        app_cb->notifyCb  = NULL;
        ALOGE("%s:Unknown AF status (%d) received",__func__,*status);
      }

    }/*(mNotifyCb && ( mMsgEnabled & CAMERA_MSG_FOCUS))*/
    else{
      ALOGE("%s:Call back not enabled",__func__);
    }

    ALOGV("autoFocusEvent: X");
    return ret;

}

status_t QCameraHardwareInterface::cancelPicture()
{
    ALOGV("cancelPicture: E");
    status_t ret = MM_CAMERA_OK;
    Mutex::Autolock lock(mLock);

    switch(mPreviewState) {
        case QCAMERA_HAL_PREVIEW_STOPPED:
        case QCAMERA_HAL_PREVIEW_START:
        case QCAMERA_HAL_PREVIEW_STARTED:
            break;
        case QCAMERA_HAL_RECORDING_STARTED:
            if(mStateLiveshot && (mStreamLiveSnap != NULL)) {
                mStreamLiveSnap->stop();
                mStateLiveshot = false;
            }
            break;
        case QCAMERA_HAL_TAKE_PICTURE:
            ret = cancelPictureInternal();
            break;
        default:
            break;
    }
    ALOGV("cancelPicture: X");
    return ret;
}

status_t QCameraHardwareInterface::cancelPictureInternal()
{
    ALOGI("cancelPictureInternal: E");
    status_t ret = MM_CAMERA_OK;
    // Do not need Snapshot callback to up layer any more
    mSnapCbDisabled = true;
    //we should make sure that snap_jpeg_cb has finished
    if(mStreamSnap){
        mSnapJpegCbLock.lock();
        if(mSnapJpegCbRunning != false){
            ALOGE("%s: wait snapshot_jpeg_cb() to finish.", __func__);
            mSnapJpegCbWait.wait(mSnapJpegCbLock);
            ALOGE("%s: finish waiting snapshot_jpeg_cb(). ", __func__);
        }
        mSnapJpegCbLock.unlock();
    }
    ALOGI("%s: mCameraState=%d.", __func__, mCameraState);
    if(mCameraState != CAMERA_STATE_READY) {
        if(mStreamSnap) {
            mStreamSnap->stop();
            mCameraState = CAMERA_STATE_SNAP_STOP_CMD_SENT;
        }
    } else {
        ALOGE("%s: Cannot process cancel picture as snapshot is already done",__func__);
    }
    ALOGV("cancelPictureInternal: X");
    return ret;
}

void QCameraHardwareInterface::pausePreviewForSnapshot()
{
    stopPreviewInternal( );
}
status_t QCameraHardwareInterface::resumePreviewAfterSnapshot()
{
    status_t ret = NO_ERROR;
    ret = mStreamDisplay->start();
    return ret;
}

void liveshot_callback(mm_camera_ch_data_buf_t *recvd_frame,
                                void *user_data)
{
    QCameraHardwareInterface *pme = (QCameraHardwareInterface *)user_data;
    cam_ctrl_dimension_t dim;
    int mJpegMaxSize;
    int mNuberOfVFEOutputs = 0;
    status_t ret;
    ALOGV("%s: E", __func__);

    if (!pme->mStateLiveshot) {
        ALOGE("%s: Returning Buffer. Picture Cancelled", __func__);
        return;
    }

    ALOGV("Live Snapshot Enabled");
    if (pme->mStreamLiveSnap){
        ALOGE("%s:Deleting old Snapshot stream instance",__func__);
        QCameraStream_Snapshot::deleteInstance (pme->mStreamLiveSnap);
        pme->mStreamLiveSnap = NULL;
    }

    pme->mStreamLiveSnap = (QCameraStream_Snapshot*)QCameraStream_Snapshot::createInstance(pme->mCameraId,
                                                       pme->myMode);

    if (!pme->mStreamLiveSnap) {
        ALOGE("%s: error - can't creat snapshot stream!", __func__);
        cam_evt_buf_done(pme->mCameraId, recvd_frame);
        return ;
    }
    pme->mStreamLiveSnap->setModeLiveSnapshot(true);
    pme->mStreamLiveSnap->setHALCameraControl(pme);

    ret = ((QCameraStream_Snapshot*)(pme->mStreamLiveSnap))->takePictureLiveshot(recvd_frame);
    if (MM_CAMERA_OK != ret) {
        ALOGE("%s: Error : returned from takePictureLiveshot",__func__);
        return;
    }
    pme->setCAFLockCancel();
    ALOGV("%s: X", __func__);

}

status_t  QCameraHardwareInterface::takePicture()
{
    ALOGV("takePicture: E");
    status_t ret = MM_CAMERA_OK;
    int mNuberOfVFEOutputs = 0;
    Mutex::Autolock lock(mLock);
    bool hdr;
    int  frm_num = 1, rc = 0;
    int  exp[MAX_HDR_EXP_FRAME_NUM];
	
    mSnapJpegCbLock.lock();
    if(mSnapJpegCbRunning==true){ // This flag is set true in snapshot_jpeg_cb
       ALOGE("%s: wait snapshot_jpeg_cb() to finish to proceed with the next take picture", __func__);
       mSnapJpegCbWait.wait(mSnapJpegCbLock);
       ALOGE("%s: finish waiting snapshot_jpeg_cb() ", __func__);
    }
    mSnapJpegCbLock.unlock();

    // if we have liveSnapshot instance,
    // we need to delete it here to release teh channel it acquires
    if (NULL != mStreamLiveSnap) {
      delete(mStreamLiveSnap);
      mStreamLiveSnap = NULL;
    }

    mSnapCbDisabled = false;

    if(QCAMERA_HAL_RECORDING_STARTED != mPreviewState){
      isp3a_af_mode_t afMode = getAutoFocusMode(mParameters);
      if (!mFlashCond)
      {
        mFlashCond = getFlashCondition();
      }
      ALOGV("%s: Flash Contidion %d", __func__, mFlashCond);
      if(mFlashCond) {
        mRestartPreview = true;
        pausePreviewForZSL();
      }
      mFlashCond = false;
    }

    rc = cam_config_set_parm(mCameraId, MM_CAMERA_PARM_LG_CAF_LOCK, NULL);
    if(rc<0)
    {
      ALOGE("%s: Failed to Check MM_CAMERA_PARM_LG_CAF_LOCK, rc %d", __func__, rc);
    }
    hdr = getHdrInfoAndSetExp(MAX_HDR_EXP_FRAME_NUM, &frm_num, exp);
	
    mStreamSnap->resetSnapshotCounters();
    mStreamSnap->InitHdrInfoForSnapshot(hdr, frm_num, exp);
    switch(mPreviewState) {
    case QCAMERA_HAL_PREVIEW_STARTED:
        //set the fullsize liveshot to false
        mFullLiveshotEnabled = false;
        setFullLiveshot();
        mStreamSnap->setFullSizeLiveshot(false);
        if (isZSLMode()) {
            if (mStreamSnap != NULL) {
                pausePreviewForZSL();
                ret = mStreamSnap->takePictureZSL();
                if (ret != MM_CAMERA_OK) {
                    ALOGE("%s: Error taking ZSL snapshot!", __func__);
                    ret = BAD_VALUE;
                }
            }
            else {
                ALOGE("%s: ZSL stream not active! Failure!!", __func__);
                ret = BAD_VALUE;
            }
            return ret;
        }
        /*prepare snapshot, e.g LED*/
        takePicturePrepareHardware( );
        /* There's an issue where we have a glimpse of corrupted data between
           a time we stop a preview and display the postview. It happens because
           when we call stopPreview we deallocate the preview buffers hence overlay
           displays garbage value till we enqueue postview buffer to be displayed.
           Hence for temporary fix, we'll do memcopy of the last frame displayed and
           queue it to overlay*/
        // mzhu storePreviewFrameForPostview();

        /* stop preview */
        pausePreviewForSnapshot();

        /* call Snapshot start() :*/
        ret =  mStreamSnap->start();
        if (MM_CAMERA_OK != ret){
            /* mzhu: fix me, restore preview */
            ALOGE("%s: error - can't start Snapshot stream!", __func__);
            return BAD_VALUE;
        }

        if(MM_CAMERA_OK == ret)
            mCameraState = CAMERA_STATE_SNAP_START_CMD_SENT;
        else
            mCameraState = CAMERA_STATE_ERROR;
        mPreviewState = QCAMERA_HAL_TAKE_PICTURE;
        break;
      case QCAMERA_HAL_TAKE_PICTURE:
          break;
    case QCAMERA_HAL_PREVIEW_STOPPED:
    case QCAMERA_HAL_PREVIEW_START:
      ret = UNKNOWN_ERROR;
      break;
    case QCAMERA_HAL_RECORDING_STARTED:
      if(mStateLiveshot) {
          ALOGE("takePicture : Duplicate TakePicture Call");
          return ret;
      }
      if (canTakeFullSizeLiveshot()) {
        ALOGV(" Calling takeFullSizeLiveshot");
        takeFullSizeLiveshot();
      }else{
        ret = cam_config_get_parm(mCameraId, MM_CAMERA_PARM_VFE_OUTPUT_ENABLE,
                     &mNuberOfVFEOutputs);
        if (ret != MM_CAMERA_OK) {
           ALOGE("get parm MM_CAMERA_PARM_VFE_OUTPUT_ENABLE  failed");
           ret = BAD_VALUE;
        }
        if (mNuberOfVFEOutputs == 1){
           (void) cam_evt_register_buf_notify(mCameraId, MM_CAMERA_CH_PREVIEW,
                                                    liveshot_callback,
                                                    MM_CAMERA_REG_BUF_CB_COUNT,
                                                    1,
                                                    this);
        } else {
           (void) cam_evt_register_buf_notify(mCameraId, MM_CAMERA_CH_VIDEO,
                                                    liveshot_callback,
                                                    MM_CAMERA_REG_BUF_CB_COUNT,
                                                    1,
                                                    this);
        }
      }
      mStateLiveshot = true;
      break;
    default:
        ret = UNKNOWN_ERROR;
        break;
    }
    ALOGV("takePicture: X");
    return ret;
}

bool QCameraHardwareInterface::canTakeFullSizeLiveshot() {
    bool ret;
    if (mFullLiveshotEnabled && !isLowPowerCamcorder()) {
      /* Full size liveshot enabled. */

      /* If Picture size is same as video size, switch to Video size
       * live snapshot */
      if ((mDimension.picture_width == mVideoWidth) &&
          (mDimension.picture_height == mVideoHeight)) {
        return false;
      }

      if (mDisEnabled) {
       /* If DIS is enabled and Picture size is
        * less than (video size + 10% DIS Margin)
        * then fall back to Video size liveshot. */
        if ((mDimension.picture_width <
               (int)(mVideoWidth * 1.1)) ||
             (mDimension.picture_height <
               (int)(mVideoHeight * 1.1))) {
          ret = false;
        } else {
          /* Go with Full size live snapshot. */
          ret = true;
        }
      } else {
        /* DIS Disabled. Go with Full size live snapshot */
        ret = true;
      }
    } else {
      /* Full size liveshot disabled. Fallback to Video size liveshot. */
      ret = false;
    }

    return ret;
}

status_t QCameraHardwareInterface::takeFullSizeLiveshot()
{
    status_t ret = NO_ERROR;
    if (mStreamLiveSnap){
        ALOGV("%s:Deleting old Snapshot stream instance",__func__);
        QCameraStream_Snapshot::deleteInstance (mStreamLiveSnap);
        mStreamLiveSnap = NULL;
    }
    mStreamLiveSnap = QCameraStream_Snapshot::createInstance(mCameraId, myMode);

    if (!mStreamLiveSnap) {
        ALOGE("%s: error - can't creat snapshot stream!", __func__);
        /* mzhu: fix me, restore preview */
        return BAD_VALUE;
    }

    /* Store HAL object in snapshot stream Object */
    mStreamLiveSnap->setHALCameraControl(this);

    mStreamLiveSnap->setFullSizeLiveshot(true);

    /* Call snapshot init*/
    ret =  mStreamLiveSnap->init();
    if (MM_CAMERA_OK != ret){
        ALOGE("%s: error - can't init Snapshot stream!", __func__);
        return BAD_VALUE;
    }

    /* call Snapshot start() :*/
    mStreamLiveSnap->resetSnapshotCounters( );
    ret =  mStreamLiveSnap->start();
    if (MM_CAMERA_OK != ret){
        /* mzhu: fix me, restore preview */
        ALOGE("%s: error - can't start Snapshot stream!", __func__);
        return BAD_VALUE;
    }
    return ret;
}

status_t  QCameraHardwareInterface::takeLiveSnapshot()
{
    status_t ret = NO_ERROR;
    ALOGV("takeLiveSnapshot: E");
    mStreamRecord->takeLiveSnapshot();
    ALOGV("takeLiveSnapshot: X");
    return ret;
}

status_t QCameraHardwareInterface::autoFocus()
{
    ALOGV("autoFocus: E");
    status_t ret = NO_ERROR;
    mFlashCond = getFlashCondition();
    ALOGV("autoFocus: mFlashCond = %d", mFlashCond);
    Mutex::Autolock lock(mLock);
    ALOGV("autoFocus: Got lock");
    bool status = true;
    isp3a_af_mode_t afMode = getAutoFocusMode(mParameters);

    Mutex::Autolock afLock(mAutofocusLock);

    if(mAutoFocusRunning==true){
      ALOGV("%s:AF already running should not have got this call",__func__);
      return NO_ERROR;
    }

    if (afMode == AF_MODE_MAX) {
      /* This should never happen. We cannot send a
       * callback notifying error from this place because
       * the CameraService has called this function after
       * acquiring the lock. So if we try to issue a callback
       * from this place, the callback will try to acquire
       * the same lock in CameraService and it will result
       * in deadlock. So, let the call go in to the lower
       * layer. The lower layer will anyway return error if
       * the autofocus is not supported or if the focus
       * value is invalid.
       * Just print out the error. */
      ALOGE("%s:Invalid AF mode (%d)", __func__, afMode);
    }

    ALOGV("%s:AF start (mode %d)", __func__, afMode);
    if(MM_CAMERA_OK != cam_ops_action(mCameraId, true,
                                    MM_CAMERA_OPS_FOCUS, &afMode)) {
      ALOGE("%s: AF command failed err:%d error %s",
           __func__, errno, strerror(errno));
      return UNKNOWN_ERROR;
    }

    mAutoFocusRunning = true;
    ALOGV("autoFocus: X");
    return ret;
}

status_t QCameraHardwareInterface::cancelAutoFocus()
{
    ALOGV("cancelAutoFocus: E");
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mLock);

/**************************************************************
  BEGIN MUTEX CODE
*************************************************************/

    mAutofocusLock.lock();
    if(mAutoFocusRunning || mNeedToUnlockCaf) {
      ALOGV("%s:Af either running or CAF needs unlocking", __func__);
      mNeedToUnlockCaf = false;
      mAutoFocusRunning = false;
      mAutofocusLock.unlock();

    }else/*(!mAutoFocusRunning)*/{

      mAutofocusLock.unlock();
      ALOGV("%s:Af not running",__func__);
      return NO_ERROR;
    }
/**************************************************************
  END MUTEX CODE
*************************************************************/


    if(MM_CAMERA_OK!=cam_ops_action(mCameraId,false,MM_CAMERA_OPS_FOCUS,NULL )) {
      ALOGE("%s: AF command failed err:%d error %s",__func__, errno,strerror(errno));
    }

    ALOGV("cancelAutoFocus: X");
    return NO_ERROR;
}

#if 0 //mzhu
/*==========================================================================
 * FUNCTION    - prepareSnapshotAndWait -
 *
 * DESCRIPTION:  invoke preparesnapshot and wait for it done
                 it can be called within takepicture, so no need
                 to grab mLock.
 *=========================================================================*/
void QCameraHardwareInterface::prepareSnapshotAndWait()
{
    ALOGI("prepareSnapshotAndWait: E");
    int rc = 0;
    /*To Do: call mm camera preparesnapshot */
    if(!rc ) {
        mPreparingSnapshot = true;
        pthread_mutex_lock(&mAsyncCmdMutex);
        pthread_cond_wait(&mAsyncCmdWait, &mAsyncCmdMutex);
        pthread_mutex_unlock(&mAsyncCmdMutex);
        mPreparingSnapshot = false;
    }
    ALOGI("prepareSnapshotAndWait: X");
}
#endif //mzhu

/*==========================================================================
 * FUNCTION    - processprepareSnapshotEvent -
 *
 * DESCRIPTION:  Process the event of preparesnapshot done msg
                 unblock prepareSnapshotAndWait( )
 *=========================================================================*/
void QCameraHardwareInterface::processprepareSnapshotEvent(cam_ctrl_status_t *status)
{
    ALOGV("processprepareSnapshotEvent: E");
    pthread_mutex_lock(&mAsyncCmdMutex);
    pthread_cond_signal(&mAsyncCmdWait);
    pthread_mutex_unlock(&mAsyncCmdMutex);
    ALOGV("processprepareSnapshotEvent: X");
}

void QCameraHardwareInterface::roiEvent(fd_roi_t roi,app_notify_cb_t *app_cb)
{
    ALOGV("roiEvent: E");

    if(mStreamDisplay) mStreamDisplay->notifyROIEvent(roi);
#if 0 //TODO: move to preview obj
    mCallbackLock.lock();
    data_callback mcb = mDataCb;
    void *mdata = mCallbackCookie;
    int msgEnabled = mMsgEnabled;
    mCallbackLock.unlock();

    mMetaDataWaitLock.lock();
    if (mFaceDetectOn == true && mSendMetaData == true) {
        mSendMetaData = false;
        int faces_detected = roi.rect_num;
        int max_faces_detected = MAX_ROI * 4;
        int array[max_faces_detected + 1];

        array[0] = faces_detected * 4;
        for (int i = 1, j = 0;j < MAX_ROI; j++, i = i + 4) {
            if (j < faces_detected) {
                array[i]   = roi.faces[j].x;
                array[i+1] = roi.faces[j].y;
                array[i+2] = roi.faces[j].dx;
                array[i+3] = roi.faces[j].dy;
            } else {
                array[i]   = -1;
                array[i+1] = -1;
                array[i+2] = -1;
                array[i+3] = -1;
            }
        }
        if(mMetaDataHeap != NULL){
            ALOGV("mMetaDataHEap is non-NULL");
            memcpy((uint32_t *)mMetaDataHeap->mHeap->base(), (uint32_t *)array, (sizeof(int)*(MAX_ROI*4+1)));
            mMetaDataWaitLock.unlock();

            if  (mcb != NULL && (msgEnabled & CAMERA_MSG_META_DATA)) {
                mcb(CAMERA_MSG_META_DATA, mMetaDataHeap->mBuffers[0], mdata);
            }
        } else {
            mMetaDataWaitLock.unlock();
            ALOGE("runPreviewThread mMetaDataHeap is NULL");
        }
    } else {
        mMetaDataWaitLock.unlock();
    }
#endif // mzhu
    ALOGV("roiEvent: X");
}


void QCameraHardwareInterface::handleZoomEventForSnapshot(void)
{
    mm_camera_ch_crop_t v4l2_crop;


    ALOGV("%s: E", __func__);

    memset(&v4l2_crop,0,sizeof(v4l2_crop));
    v4l2_crop.ch_type=MM_CAMERA_CH_SNAPSHOT;

    ALOGV("%s: Fetching crop info", __func__);
    cam_config_get_parm(mCameraId,MM_CAMERA_PARM_CROP,&v4l2_crop);

    ALOGV("%s: Crop info received for main: %d, %d, %d, %d ", __func__,
         v4l2_crop.snapshot.main_crop.left,
         v4l2_crop.snapshot.main_crop.top,
         v4l2_crop.snapshot.main_crop.width,
         v4l2_crop.snapshot.main_crop.height);
    ALOGV("%s: Crop info received for thumbnail: %d, %d, %d, %d ",__func__,
         v4l2_crop.snapshot.thumbnail_crop.left,
         v4l2_crop.snapshot.thumbnail_crop.top,
         v4l2_crop.snapshot.thumbnail_crop.width,
         v4l2_crop.snapshot.thumbnail_crop.height);

    if(mStreamSnap) {
        ALOGV("%s: Setting crop info for snapshot", __func__);
        memcpy(&(mStreamSnap->mCrop), &v4l2_crop, sizeof(v4l2_crop));
    }
    if(mFullLiveshotEnabled && mStreamLiveSnap){
        ALOGV("%s: Setting crop info for snapshot", __func__);
        memcpy(&(mStreamLiveSnap->mCrop), &v4l2_crop, sizeof(v4l2_crop));
    }
    ALOGV("%s: X", __func__);
}

void QCameraHardwareInterface::handleZoomEventForPreview(app_notify_cb_t *app_cb)
{
    mm_camera_ch_crop_t v4l2_crop;

    ALOGV("%s: E", __func__);

    /*regular zooming or smooth zoom stopped*/
    if (!mSmoothZoomRunning && mPreviewWindow) {
        memset(&v4l2_crop, 0, sizeof(v4l2_crop));
        v4l2_crop.ch_type = MM_CAMERA_CH_PREVIEW;

        ALOGV("%s: Fetching crop info", __func__);
        cam_config_get_parm(mCameraId,MM_CAMERA_PARM_CROP,&v4l2_crop);

        ALOGV("%s: Crop info received: %d, %d, %d, %d ", __func__,
             v4l2_crop.crop.left,
             v4l2_crop.crop.top,
             v4l2_crop.crop.width,
             v4l2_crop.crop.height);

        mPreviewWindow->set_crop(mPreviewWindow,
                        v4l2_crop.crop.left,
                        v4l2_crop.crop.top,
                        v4l2_crop.crop.left + v4l2_crop.crop.width,
                        v4l2_crop.crop.top + v4l2_crop.crop.height);
        ALOGV("%s: Done setting crop", __func__);
        ALOGV("%s: Currrent zoom :%d",__func__, mCurrentZoom);
    }

    ALOGV("%s: X", __func__);
}

void QCameraHardwareInterface::zoomEvent(cam_ctrl_status_t *status, app_notify_cb_t *app_cb)
{
    ALOGV("zoomEvent: state:%d E",mPreviewState);
    switch (mPreviewState) {
    case QCAMERA_HAL_PREVIEW_STOPPED:
        break;
    case QCAMERA_HAL_PREVIEW_START:
        break;
    case QCAMERA_HAL_PREVIEW_STARTED:
        handleZoomEventForPreview(app_cb);
        if (isZSLMode())
          handleZoomEventForSnapshot();
        break;
    case QCAMERA_HAL_RECORDING_STARTED:
        handleZoomEventForPreview(app_cb);
        if (mFullLiveshotEnabled)
            handleZoomEventForSnapshot();
        break;
    case QCAMERA_HAL_TAKE_PICTURE:
        if(isZSLMode())
            handleZoomEventForPreview(app_cb);
        handleZoomEventForSnapshot();
        break;
    default:
        break;
    }
    ALOGV("zoomEvent: X");
}

void QCameraHardwareInterface::dumpFrameToFile(const void * data, uint32_t size, char* name, char* ext, int index)
{
#if 0
    char buf[32], value[PROPERTY_VALUE_MAX];
    int file_fd, enabled = 0;
    static int i = 0 ;
    property_get("persist.camera.dumpimage", value, "0");
    enabled = atoi(value);

    if ( data != NULL && enabled) {
        char * str;
        snprintf(buf, sizeof(buf), "/data/%s_%d.%s", name, index, ext);
        ALOGE("%s size =%d", buf, size);
        file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        write(file_fd, data, size);
        close(file_fd);
        i++;
    }
#endif
}

void QCameraHardwareInterface::dumpFrameToFile(struct msm_frame* newFrame,
  HAL_cam_dump_frm_type_t frm_type)
{
#if 0
  int32_t enabled = 0;
  int frm_num;
  uint32_t  skip_mode;
  char value[PROPERTY_VALUE_MAX];
  char buf[32];
  int main_422 = 1;
  property_get("persist.camera.dumpimg", value, "0");
  enabled = atoi(value);

  ALOGV(" newFrame =%p, frm_type = %d", newFrame, frm_type);
  if(enabled & HAL_DUMP_FRM_MASK_ALL) {
    if((enabled & frm_type) && newFrame) {
      frm_num = ((enabled & 0xffff0000) >> 16);
      if(frm_num == 0) frm_num = 10; /*default 10 frames*/
      if(frm_num > 256) frm_num = 256; /*256 buffers cycle around*/
      skip_mode = ((enabled & 0x0000ff00) >> 8);
      if(skip_mode == 0) skip_mode = 1; /*no -skip */

      if( mDumpSkipCnt % skip_mode == 0) {
        if (mDumpFrmCnt >= 0 && mDumpFrmCnt <= frm_num) {
          int w, h;
          int file_fd;
          switch (frm_type) {
          case  HAL_DUMP_FRM_PREVIEW:
            w = mDimension.display_width;
            h = mDimension.display_height;
            snprintf(buf, sizeof(buf), "/data/%dp_%dx%d.yuv", mDumpFrmCnt, w, h);
            file_fd = open(buf, O_RDWR | O_CREAT, 0777);
            break;
          case HAL_DUMP_FRM_VIDEO:
            w = mVideoWidth;
            h = mVideoHeight;
            snprintf(buf, sizeof(buf),"/data/%dv_%dx%d.yuv", mDumpFrmCnt, w, h);
            file_fd = open(buf, O_RDWR | O_CREAT, 0777);
            break;
          case HAL_DUMP_FRM_MAIN:
            w = mDimension.picture_width;
            h = mDimension.picture_height;
            snprintf(buf, sizeof(buf), "/data/%dm_%dx%d.yuv", mDumpFrmCnt, w, h);
            file_fd = open(buf, O_RDWR | O_CREAT, 0777);
            if (mDimension.main_img_format == CAMERA_YUV_422_NV16 ||
                mDimension.main_img_format == CAMERA_YUV_422_NV61)
              main_422 = 2;
            break;
          case HAL_DUMP_FRM_THUMBNAIL:
            w = mDimension.ui_thumbnail_width;
            h = mDimension.ui_thumbnail_height;
            snprintf(buf, sizeof(buf),"/data/%dt_%dx%d.yuv", mDumpFrmCnt, w, h);
            file_fd = open(buf, O_RDWR | O_CREAT, 0777);
            break;
          default:
            w = h = 0;
            file_fd = -1;
            break;
          }

          if (file_fd < 0) {
            ALOGE("%s: cannot open file:type=%d\n", __func__, frm_type);
          } else {
            ALOGV("%s: %d %d", __func__, newFrame->y_off, newFrame->cbcr_off);
            write(file_fd, (const void *)(newFrame->buffer+newFrame->y_off), w * h);
            write(file_fd, (const void *)
              (newFrame->buffer + newFrame->cbcr_off), w * h / 2 * main_422);
            close(file_fd);
            ALOGV("dump %s", buf);
          }
        } else if(frm_num == 256){
          mDumpFrmCnt = 0;
        }
        mDumpFrmCnt++;
      }
      mDumpSkipCnt++;
    }
  }  else {
    mDumpFrmCnt = 0;
  }
#endif
}

status_t QCameraHardwareInterface::setPreviewWindow(preview_stream_ops_t* window)
{
    status_t retVal = NO_ERROR;
    ALOGV(" %s: E mPreviewState = %d, mStreamDisplay = %p", __FUNCTION__, mPreviewState, mStreamDisplay);
    if( window == NULL) {
        ALOGE("%s:Received Setting NULL preview window", __func__);
    }
    Mutex::Autolock lock(mLock);
    switch(mPreviewState) {
    case QCAMERA_HAL_PREVIEW_START:
        mPreviewWindow = window;
        if(mPreviewWindow) {
            /* we have valid surface now, start preview */
            retVal = startPreview2();
            if(retVal == NO_ERROR)
                mPreviewState = QCAMERA_HAL_PREVIEW_STARTED;
            ALOGV("%s:  startPreview2 done, mPreviewState = %d", __func__, mPreviewState);
        } else
            ALOGE("%s: null window received, mPreviewState = %d", __func__, mPreviewState);
        break;
    case QCAMERA_HAL_PREVIEW_STARTED:
        /* new window comes */
        ALOGE("%s: bug, cannot handle new window in started state", __func__);
        //retVal = UNKNOWN_ERROR;
        break;
    case QCAMERA_HAL_PREVIEW_STOPPED:
    case QCAMERA_HAL_TAKE_PICTURE:
        mPreviewWindow = window;
        ALOGE("%s: mPreviewWindow = 0x%p, mStreamDisplay = 0x%p",
                                    __func__, mPreviewWindow, mStreamDisplay);
        if(mStreamDisplay)
            retVal = mStreamDisplay->setPreviewWindow(window);
        break;
    default:
        ALOGE("%s: bug, cannot handle new window in state %d", __func__, mPreviewState);
        retVal = UNKNOWN_ERROR;
        break;
    }
    ALOGV(" %s : X, mPreviewState = %d", __FUNCTION__, mPreviewState);
    return retVal;
}

int QCameraHardwareInterface::storeMetaDataInBuffers(int enable)
{
    /* this is a dummy func now. fix me later */
    mStoreMetaDataInFrame = enable;
    return 0;
}

status_t QCameraHardwareInterface::sendMappingBuf(int ext_mode, int idx, int fd,
                                                  uint32_t size, int cameraid,
                                                  mm_camera_socket_msg_type msg_type)
{
    cam_sock_packet_t packet;
    memset(&packet, 0, sizeof(cam_sock_packet_t));
    packet.msg_type = msg_type;
    packet.payload.frame_fd_map.ext_mode = ext_mode;
    packet.payload.frame_fd_map.frame_idx = idx;
    packet.payload.frame_fd_map.fd = fd;
    packet.payload.frame_fd_map.size = size;

    if ( cam_ops_sendmsg(cameraid, &packet, sizeof(cam_sock_packet_t), packet.payload.frame_fd_map.fd) <= 0 ) {
        ALOGE("%s: sending frame mapping buf msg Failed", __func__);
        return FAILED_TRANSACTION;
    }
    return NO_ERROR;
}

status_t QCameraHardwareInterface::sendUnMappingBuf(int ext_mode, int idx, int cameraid,
                                                    mm_camera_socket_msg_type msg_type)
{
    cam_sock_packet_t packet;
    memset(&packet, 0, sizeof(cam_sock_packet_t));
    packet.msg_type = msg_type;
    packet.payload.frame_fd_unmap.ext_mode = ext_mode;
    packet.payload.frame_fd_unmap.frame_idx = idx;
    if ( cam_ops_sendmsg(cameraid, &packet, sizeof(cam_sock_packet_t), 0) <= 0 ) {
        ALOGE("%s: sending frame unmapping buf msg Failed", __func__);
        return FAILED_TRANSACTION;
    }
    return NO_ERROR;
}

int QCameraHardwareInterface::allocate_ion_memory(QCameraHalHeap_t *p_camera_memory, int cnt, int ion_type)
{
  int rc = 0;
  struct ion_handle_data handle_data;

  p_camera_memory->main_ion_fd[cnt] = open("/dev/ion", O_RDONLY);
  if (p_camera_memory->main_ion_fd[cnt] < 0) {
    ALOGE("Ion dev open failed\n");
    ALOGE("Error is %s\n", strerror(errno));
    goto ION_OPEN_FAILED;
  }
  p_camera_memory->alloc[cnt].len = p_camera_memory->size;
  /* to make it page size aligned */
  p_camera_memory->alloc[cnt].len = (p_camera_memory->alloc[cnt].len + 4095) & (~4095);
  p_camera_memory->alloc[cnt].align = 4096;
  p_camera_memory->alloc[cnt].flags = ION_FLAG_CACHED;
  p_camera_memory->alloc[cnt].heap_id_mask = ion_type;

  rc = ioctl(p_camera_memory->main_ion_fd[cnt], ION_IOC_ALLOC, &p_camera_memory->alloc[cnt]);
  if (rc < 0) {
    ALOGE("ION allocation failed\n");
    goto ION_ALLOC_FAILED;
  }

  p_camera_memory->ion_info_fd[cnt].handle = p_camera_memory->alloc[cnt].handle;
  rc = ioctl(p_camera_memory->main_ion_fd[cnt], ION_IOC_SHARE, &p_camera_memory->ion_info_fd[cnt]);
  if (rc < 0) {
    ALOGE("ION map failed %s\n", strerror(errno));
    goto ION_MAP_FAILED;
  }
  p_camera_memory->fd[cnt] = p_camera_memory->ion_info_fd[cnt].fd;
  return 0;

ION_MAP_FAILED:
  handle_data.handle = p_camera_memory->ion_info_fd[cnt].handle;
  ioctl(p_camera_memory->main_ion_fd[cnt], ION_IOC_FREE, &handle_data);
ION_ALLOC_FAILED:
  close(p_camera_memory->main_ion_fd[cnt]);
  p_camera_memory->main_ion_fd[cnt] = -1;
ION_OPEN_FAILED:
  return -1;
}

int QCameraHardwareInterface::deallocate_ion_memory(QCameraHalHeap_t *p_camera_memory, int cnt)
{
  struct ion_handle_data handle_data;
  int rc = 0;

  if (p_camera_memory->main_ion_fd[cnt] > 0) {
      handle_data.handle = p_camera_memory->ion_info_fd[cnt].handle;
      ioctl(p_camera_memory->main_ion_fd[cnt], ION_IOC_FREE, &handle_data);
      close(p_camera_memory->main_ion_fd[cnt]);
      p_camera_memory->main_ion_fd[cnt] = -1;
  }
  return rc;
}

int QCameraHardwareInterface::allocate_ion_memory(QCameraStatHeap_t *p_camera_memory, int cnt, int ion_type)
{
  int rc = 0;
  struct ion_handle_data handle_data;

  p_camera_memory->main_ion_fd[cnt] = open("/dev/ion", O_RDONLY);
  if (p_camera_memory->main_ion_fd[cnt] < 0) {
    ALOGE("Ion dev open failed\n");
    ALOGE("Error is %s\n", strerror(errno));
    goto ION_OPEN_FAILED;
  }
  p_camera_memory->alloc[cnt].len = p_camera_memory->size;
  /* to make it page size aligned */
  p_camera_memory->alloc[cnt].len = (p_camera_memory->alloc[cnt].len + 4095) & (~4095);
  p_camera_memory->alloc[cnt].align = 4096;
  p_camera_memory->alloc[cnt].flags = ION_FLAG_CACHED;
  p_camera_memory->alloc[cnt].heap_id_mask = (0x1 << ion_type | 0x1 << ION_IOMMU_HEAP_ID);

  rc = ioctl(p_camera_memory->main_ion_fd[cnt], ION_IOC_ALLOC, &p_camera_memory->alloc[cnt]);
  if (rc < 0) {
    ALOGE("ION allocation failed\n");
    goto ION_ALLOC_FAILED;
  }

  p_camera_memory->ion_info_fd[cnt].handle = p_camera_memory->alloc[cnt].handle;
  rc = ioctl(p_camera_memory->main_ion_fd[cnt], ION_IOC_SHARE, &p_camera_memory->ion_info_fd[cnt]);
  if (rc < 0) {
    ALOGE("ION map failed %s\n", strerror(errno));
    goto ION_MAP_FAILED;
  }
  p_camera_memory->fd[cnt] = p_camera_memory->ion_info_fd[cnt].fd;
  return 0;

ION_MAP_FAILED:
  handle_data.handle = p_camera_memory->ion_info_fd[cnt].handle;
  ioctl(p_camera_memory->main_ion_fd[cnt], ION_IOC_FREE, &handle_data);
ION_ALLOC_FAILED:
  close(p_camera_memory->main_ion_fd[cnt]);
  p_camera_memory->main_ion_fd[cnt] = -1;
ION_OPEN_FAILED:
  return -1;
}

int QCameraHardwareInterface::cache_ops(int ion_fd,
  struct ion_flush_data *cache_data, int type)
{
  int rc = 0;
  struct ion_custom_data data;
  data.cmd = type;
  data.arg = (unsigned long)cache_data;

  rc = ioctl(ion_fd, ION_IOC_CUSTOM, &data);
  if (rc < 0)
    ALOGE("%s: Cache Invalidate failed\n", __func__);
  else
    ALOGV("%s: Cache OPs type(%d) success", __func__);

  return rc;
}

int QCameraHardwareInterface::deallocate_ion_memory(QCameraStatHeap_t *p_camera_memory, int cnt)
{
  struct ion_handle_data handle_data;
  int rc = 0;

  if (p_camera_memory->main_ion_fd[cnt] > 0) {
      handle_data.handle = p_camera_memory->ion_info_fd[cnt].handle;
      ioctl(p_camera_memory->main_ion_fd[cnt], ION_IOC_FREE, &handle_data);
      close(p_camera_memory->main_ion_fd[cnt]);
      p_camera_memory->main_ion_fd[cnt] = -1;
  }
  return rc;
}

int QCameraHardwareInterface::initHeapMem( QCameraHalHeap_t *heap,
                            int num_of_buf,
                            int buf_len,
                            int y_off,
                            int cbcr_off,
                            int pmem_type,
                            mm_cameara_stream_buf_t *StreamBuf,
                            mm_camera_buf_def_t *buf_def,
                            uint8_t num_planes,
                            uint32_t *planes
)
{
    int rc = 0;
    int i;
    int path;
    int ion_fd;
    struct msm_frame *frame;
    struct ion_flush_data cache_inv_data;
    ALOGV("Init Heap =%p. stream_buf =%p, pmem_type =%d, num_of_buf=%d. buf_len=%d, cbcr_off=%d",
         heap, StreamBuf, pmem_type, num_of_buf, buf_len, cbcr_off);
    if(num_of_buf > MM_CAMERA_MAX_NUM_FRAMES || heap == NULL ||
       mGetMemory == NULL ) {
        ALOGE("Init Heap error");
        rc = -1;
        return rc;
    }
    memset(heap, 0, sizeof(QCameraHalHeap_t));
    for (i=0; i<MM_CAMERA_MAX_NUM_FRAMES;i++) {
        heap->main_ion_fd[i] = -1;
        heap->fd[i] = -1;
    }
    heap->buffer_count = num_of_buf;
    heap->size = buf_len;
    heap->y_offset = y_off;
    heap->cbcr_offset = cbcr_off;

    if (StreamBuf != NULL) {
        StreamBuf->num = num_of_buf;
                StreamBuf->frame_len = buf_len;
        switch (pmem_type) {
            case  MSM_PMEM_MAINIMG:
            case  MSM_PMEM_RAW_MAINIMG:
                path = OUTPUT_TYPE_S;
                break;

            case  MSM_PMEM_THUMBNAIL:
                path = OUTPUT_TYPE_T;
                break;

            default:
                rc = -1;
                return rc;
        }
    }


    for(i = 0; i < num_of_buf; i++) {
#ifdef USE_ION
      if (isZSLMode())
        rc = allocate_ion_memory(heap, i, ((0x1 << CAMERA_ZSL_ION_HEAP_ID) |
         (0x1 << CAMERA_ZSL_ION_FALLBACK_HEAP_ID)));
      else
        rc = allocate_ion_memory(heap, i, ((0x1 << CAMERA_ION_HEAP_ID) |
         (0x1 << CAMERA_ION_FALLBACK_HEAP_ID)));

      if (rc < 0) {
        ALOGE("%s: ION allocation failed\n", __func__);
        break;
      }
#else
        if (pmem_type == MSM_PMEM_MAX)
            heap->fd[i] = -1;
        else {
            heap->fd[i] = open("/dev/pmem_adsp", O_RDWR|O_SYNC);
            if ( heap->fd[i] <= 0) {
                rc = -1;
                ALOGE("Open fail: heap->fd[%d] =%d", i, heap->fd[i]);
                break;
            }
        }
#endif
        heap->camera_memory[i] =  mGetMemory( heap->fd[i], buf_len, 1, (void *)this);

        if (heap->camera_memory[i] == NULL ) {
            ALOGE("Getmem fail %d: ", i);
            rc = -1;
            break;
        }

        memset(&cache_inv_data, 0, sizeof(struct ion_flush_data));
        cache_inv_data.vaddr = (void*) heap->camera_memory[i]->data;
        cache_inv_data.fd = heap->ion_info_fd[i].fd;
        cache_inv_data.handle = heap->ion_info_fd[i].handle;
        cache_inv_data.length = heap->alloc[i].len;
        ion_fd = heap->main_ion_fd[i];
        if(ion_fd > 0) {
            if(cache_ops(ion_fd, &cache_inv_data, ION_IOC_CLEAN_INV_CACHES) < 0)
                ALOGE("%s: Cache Invalidate failed\n", __func__);
            else {
                ALOGV("%s: Successful cache invalidate\n", __func__);
            }
        }

        if (StreamBuf != NULL) {
            frame = &(StreamBuf->frame[i]);
            memset(frame, 0, sizeof(struct msm_frame));
            frame->fd = heap->fd[i];
            frame->phy_offset = 0;
            frame->buffer = (uint32_t) heap->camera_memory[i]->data;
            frame->path = path;
            frame->cbcr_off =  planes[0]+heap->cbcr_offset;
            frame->y_off =  heap->y_offset;
            frame->fd_data = heap->ion_info_fd[i];
            frame->ion_alloc = heap->alloc[i];
            frame->ion_dev_fd = heap->main_ion_fd[i];
            ALOGV("%s: Buffer idx: %d  addr: %x fd: %d phy_offset: %d"
                 "cbcr_off: %d y_off: %d frame_len: %d", __func__,
                 i, (unsigned int)frame->buffer, frame->fd,
                 frame->phy_offset, cbcr_off, y_off, frame->ion_alloc.len);

            buf_def->buf.mp[i].frame = *frame;
            buf_def->buf.mp[i].frame_offset = 0;
            buf_def->buf.mp[i].num_planes = num_planes;
            /* Plane 0 needs to be set seperately. Set other planes
             * in a loop. */
            buf_def->buf.mp[i].planes[0].length = planes[0];
            buf_def->buf.mp[i].planes[0].m.userptr = frame->fd;
            buf_def->buf.mp[i].planes[0].data_offset = y_off;
            buf_def->buf.mp[i].planes[0].reserved[0] =
              buf_def->buf.mp[i].frame_offset;
            for (int j = 1; j < num_planes; j++) {
                 buf_def->buf.mp[i].planes[j].length = planes[j];
                 buf_def->buf.mp[i].planes[j].m.userptr = frame->fd;
                 buf_def->buf.mp[i].planes[j].data_offset = cbcr_off;
                 buf_def->buf.mp[i].planes[j].reserved[0] =
                     buf_def->buf.mp[i].planes[j-1].reserved[0] +
                     buf_def->buf.mp[i].planes[j-1].length;
            }
        } else {
        }

        ALOGV("heap->fd[%d] =%d, camera_memory=%p", i, heap->fd[i], heap->camera_memory[i]);
        heap->local_flag[i] = 1;
    }
    if( rc < 0) {
        releaseHeapMem(heap);
    }
    return rc;
}

int QCameraHardwareInterface::releaseHeapMem( QCameraHalHeap_t *heap)
{
	int rc = 0;
	ALOGV("Release %p", heap);
	if (heap != NULL) {

		for (int i = 0; i < heap->buffer_count; i++) {
			if(heap->camera_memory[i] != NULL) {
				heap->camera_memory[i]->release( heap->camera_memory[i] );
				heap->camera_memory[i] = NULL;
			} else if (heap->fd[i] <= 0) {
				ALOGE("impossible: amera_memory[%d] = %p, fd = %d",
				i, heap->camera_memory[i], heap->fd[i]);
			}

			if(heap->fd[i] > 0) {
				close(heap->fd[i]);
				heap->fd[i] = -1;
			}
#ifdef USE_ION
            deallocate_ion_memory(heap, i);
#endif
		}
        heap->buffer_count = 0;
        heap->size = 0;
        heap->y_offset = 0;
        heap->cbcr_offset = 0;
	}
	return rc;
}

preview_format_info_t  QCameraHardwareInterface::getPreviewFormatInfo( )
{
  return mPreviewFormatInfo;
}

void QCameraHardwareInterface::wdenoiseEvent(cam_ctrl_status_t status, void *cookie)
{
    ALOGV("wdnEvent: preview state:%d E",mPreviewState);
    if (mStreamSnap != NULL) {
        ALOGV("notifyWDNEvent to snapshot stream");
        mStreamSnap->notifyWDenoiseEvent(status, cookie);
    }
}

bool QCameraHardwareInterface::isWDenoiseEnabled()
{
    return mDenoiseValue;
}

void QCameraHardwareInterface::takePicturePrepareHardware()
{
    ALOGV("%s: E", __func__);

    /* Prepare snapshot*/
    cam_ops_action(mCameraId,
                  true,
                  MM_CAMERA_OPS_PREPARE_SNAPSHOT,
                  this);
    ALOGV("%s: X", __func__);
}

void QCameraHardwareInterface::pausePreviewForZSL()
{
    ALOGV("%s: mRestartPreview %d", __func__, mRestartPreview);
    if(mRestartPreview) {
        ALOGE("%s: Restarting Preview",__func__);
        stopPreviewInternal();
        mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;
        startPreview2();
        mPreviewState = QCAMERA_HAL_PREVIEW_STARTED;
    }
}

void QCameraHardwareInterface::pausePreviewForVideo()
{
    status_t ret = NO_ERROR;
    bool restart = false;
    cam_ctrl_dimension_t dim;

    /*  get existing preview information, by qury mm_camera*/
    memset(&dim, 0, sizeof(cam_ctrl_dimension_t));
    ret = cam_config_get_parm(mCameraId, MM_CAMERA_PARM_DIMENSION,&dim);

    if (MM_CAMERA_OK != ret) {
      ALOGE("%s: X error- can't get preview dimension!", __func__);
      return;
    }

    if (mRestartPreview) {
        mRestartPreview = false;
        ALOGV("%s: Restarting Preview. Video Size changed",__func__);
        restart |= false;
    }
    if (mRecordingHint == false) {
        ALOGV("%s: Restarting Preview. Hint not set",__func__);
        restart |= true;
    }

    if(dim.video_width != mVideoWidth || dim.video_height != mVideoHeight){
        ALOGV("%s: Restarting Preview. New Video Size set",__func__);
        restart |= true;
    }

    //VFE output1 shouldn't be greater than VFE output2.
    if( (mPreviewWidth > mVideoWidth) || (mPreviewHeight > mVideoHeight)) {
        //Set preview sizes as record sizes.
        ALOGV("Preview size %dx%d is greater than record size %dx%d,\
                resetting preview size to record size",mPreviewWidth,
                        mPreviewHeight, mVideoWidth, mVideoHeight);
        mPreviewWidth = mVideoWidth;
        mPreviewHeight = mVideoHeight;
        mParameters.setPreviewSize(mPreviewWidth, mPreviewHeight);
        restart |= true;
    }
    if (restart) {
        stopPreviewInternal();
        mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;

        // Set recording hint to true
        mRecordingHint = true;
        setRecordingHintValue(mRecordingHint);

        mDimension.display_width = mPreviewWidth;
        mDimension.display_height= mPreviewHeight;
        mDimension.orig_video_width = mVideoWidth;
        mDimension.orig_video_height = mVideoHeight;
        mDimension.video_width = mVideoWidth;
        mDimension.video_height = mVideoHeight;

        // start preview again
        mPreviewState = QCAMERA_HAL_PREVIEW_START;
        if (startPreview2() == NO_ERROR){
            mPreviewState = QCAMERA_HAL_PREVIEW_STARTED;
        }else{
            ALOGE("%s: Restart for Video Failed",__func__);
        }
    }
}

/**/
bool QCameraHardwareInterface::getHdrInfoAndSetExp( int max_num_frm, int *num_frame, int *exp)
{
    bool rc = false;

    if (mHdrMode == HDR_MODE && num_frame != NULL && exp != NULL &&
      mRecordingHint != true &&
      mPreviewState != QCAMERA_HAL_RECORDING_STARTED ) {
        int ret = 0;
        *num_frame = 1;
        exp_bracketing_t temp;
        memset(&temp, 0, sizeof(exp_bracketing_t));
        ret = cam_config_get_parm(mCameraId, MM_CAMERA_PARM_HDR, (void *)&temp );
        if (ret == NO_ERROR && max_num_frm > 0) {
            /*set as AE Bracketing mode*/
            temp.hdr_enable = false;
            temp.mode = HDR_MODE;
            temp.total_hal_frames = temp.total_frames;
            ret = native_set_parms(MM_CAMERA_PARM_HDR,
                                   sizeof(exp_bracketing_t), (void *)&temp);
            if (ret) {
                char *val, *exp_value, *prev_value;
                int i;
                exp_value = (char *) temp.values;
                i = 0;
                val = strtok_r(exp_value,",", &prev_value);
                while (val != NULL ){
                  exp[i++] = atoi(val);
                  if(i >= max_num_frm )
                    break;
                  val = strtok_r(NULL, ",", &prev_value);
                }
                *num_frame =temp.total_frames;
                rc = true;
            }
        } else {
            temp.total_frames = 1;
        }
        /* Application waits until this many snapshots before restarting preview */
        mParameters.set("num-snaps-per-shutter", 2);
    }
    return rc;
}

void QCameraHardwareInterface::hdrEvent(cam_ctrl_status_t status, void *cookie)
{
    QCameraStream * snapStream = (QCameraStream *)cookie;
    ALOGV("HdrEvent: preview state: E");
    if (snapStream != NULL && mStreamSnap != NULL) {
        ALOGV("HdrEvent to snapshot stream");
        snapStream->notifyHdrEvent(status, cookie);
    }
}

}; // namespace android
