/*
** Copyright (c) 2011 The Linux Foundation. All rights reserved.
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

//#define LOG_NDEBUG 0
#define LOG_NIDEBUG 0
#define LOG_TAG "QCameraHWI_Record"
#include <utils/Log.h>
#include <utils/threads.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "QCameraStream.h"


#define LIKELY(exp)   __builtin_expect(!!(exp), 1)
#define UNLIKELY(exp) __builtin_expect(!!(exp), 0)

/* QCameraStream_record class implementation goes here*/
/* following code implement the video streaming capture & encoding logic of this class*/
// ---------------------------------------------------------------------------
// QCameraStream_record createInstance()
// ---------------------------------------------------------------------------
namespace android {


QCameraStream* QCameraStream_record::createInstance(int cameraId,
                                      camera_mode_t mode)
{
  LOGV("%s: BEGIN", __func__);
  QCameraStream* pme = new QCameraStream_record(cameraId, mode);
  LOGV("%s: END", __func__);
  return pme;
}

// ---------------------------------------------------------------------------
// QCameraStream_record deleteInstance()
// ---------------------------------------------------------------------------
void QCameraStream_record::deleteInstance(QCameraStream *ptr)
{
  LOGV("%s: BEGIN", __func__);
  if (ptr){
    ptr->release();
    delete ptr;
    ptr = NULL;
  }
  LOGV("%s: END", __func__);
}

// ---------------------------------------------------------------------------
// QCameraStream_record Constructor
// ---------------------------------------------------------------------------
QCameraStream_record::QCameraStream_record(int cameraId,
                                           camera_mode_t mode)
  :QCameraStream(cameraId,mode),
  mDebugFps(false)
{
  mHalCamCtrl = NULL;
  char value[PROPERTY_VALUE_MAX];
  LOGV("%s: BEGIN", __func__);

  property_get("persist.debug.sf.showfps", value, "0");
  mDebugFps = atoi(value);

  LOGV("%s: END", __func__);
}

// ---------------------------------------------------------------------------
// QCameraStream_record Destructor
// ---------------------------------------------------------------------------
QCameraStream_record::~QCameraStream_record() {
  LOGV("%s: BEGIN", __func__);
  if(mActive) {
    stop();
  }
  if(mInit) {
    release();
  }
  mInit = false;
  mActive = false;
  LOGV("%s: END", __func__);

}

// ---------------------------------------------------------------------------
// QCameraStream_record Callback from mm_camera
// ---------------------------------------------------------------------------
static void record_notify_cb(mm_camera_ch_data_buf_t *bufs_new,
                              void *user_data)
{
  QCameraStream_record *pme = (QCameraStream_record *)user_data;
  mm_camera_ch_data_buf_t *bufs_used = 0;
  LOGV("%s: BEGIN", __func__);

  /*
  * Call Function Process Video Data
  */
  pme->processRecordFrame(bufs_new);
  LOGV("%s: END", __func__);
}

// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
status_t QCameraStream_record::init()
{
  status_t ret = NO_ERROR;
  LOGV("%s: BEGIN", __func__);

  /*
  *  Acquiring Video Channel
  */
  ret = QCameraStream::initChannel (mCameraId, MM_CAMERA_CH_VIDEO_MASK);
  if (NO_ERROR!=ret) {
    LOGE("%s ERROR: Can't init native cammera preview ch\n",__func__);
    return ret;
  }

  /*
  * Register the Callback with camera
  */
  (void) cam_evt_register_buf_notify(mCameraId, MM_CAMERA_CH_VIDEO,
                                            record_notify_cb,
                                            MM_CAMERA_REG_BUF_CB_INFINITE,
                                            0,
                                            this);

  mInit = true;
  LOGV("%s: END", __func__);
  return ret;
}
// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------

status_t QCameraStream_record::start()
{
  status_t ret = NO_ERROR;
  LOGV("%s: BEGIN", __func__);

  Mutex::Autolock lock(mStopCallbackLock);
  if(!mInit) {
    LOGE("%s ERROR: Record buffer not registered",__func__);
    return BAD_VALUE;
  }

  setFormat(MM_CAMERA_CH_VIDEO_MASK);
  //mRecordFreeQueueLock.lock();
  //mRecordFreeQueue.clear();
  //mRecordFreeQueueLock.unlock();
  /*
  *  Allocating Encoder Frame Buffers
  */
  ret = initEncodeBuffers();
  if (NO_ERROR!=ret) {
    LOGE("%s ERROR: Buffer Allocation Failed\n",__func__);
    goto error;
  }

  ret = cam_config_prepare_buf(mCameraId, &mRecordBuf);
  if(ret != MM_CAMERA_OK) {
    LOGV("%s ERROR: Reg Record buf err=%d\n", __func__, ret);
    ret = BAD_VALUE;
    goto error;
  }else{
    ret = NO_ERROR;
  }

  /*
  * Start Video Streaming
  */
  ret = cam_ops_action(mCameraId, TRUE, MM_CAMERA_OPS_VIDEO, 0);
  if (MM_CAMERA_OK != ret) {
    LOGE ("%s ERROR: Video streaming start err=%d\n", __func__, ret);
    ret = BAD_VALUE;
    goto error;
  }else{
    LOGE("%s : Video streaming Started",__func__);
    ret = NO_ERROR;
  }
  mActive = true;
  LOGV("%s: END", __func__);
  return ret;

error:
  releaseEncodeBuffer();
  LOGV("%s: END", __func__);
  return ret;
}

void QCameraStream_record::releaseEncodeBuffer() {
  for(int cnt = 0; cnt < mHalCamCtrl->mRecordingMemory.buffer_count; cnt++) {
    if (NO_ERROR !=
      mHalCamCtrl->sendUnMappingBuf(MSM_V4L2_EXT_CAPTURE_MODE_VIDEO, cnt,
      mCameraId, CAM_SOCK_MSG_TYPE_FD_UNMAPPING))
      LOGE("%s: Unmapping Video Data Failed", __func__);

    if (mHalCamCtrl->mStoreMetaDataInFrame) {
      struct encoder_media_buffer_type * packet =
          (struct encoder_media_buffer_type  *)
          mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->data;
      native_handle_delete(const_cast<native_handle_t *>(packet->meta_handle));
      mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->release(
        mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]);

    }
    mHalCamCtrl->mRecordingMemory.camera_memory[cnt]->release(
      mHalCamCtrl->mRecordingMemory.camera_memory[cnt]);
    close(mHalCamCtrl->mRecordingMemory.fd[cnt]);
    mHalCamCtrl->mRecordingMemory.fd[cnt] = -1;

#ifdef USE_ION
    mHalCamCtrl->deallocate_ion_memory(&mHalCamCtrl->mRecordingMemory, cnt);
#endif
  }
  memset(&mHalCamCtrl->mRecordingMemory, 0, sizeof(mHalCamCtrl->mRecordingMemory));
  //mNumRecordFrames = 0;
  delete[] recordframes;
  if (mRecordBuf.video.video.buf.mp)
    delete[] mRecordBuf.video.video.buf.mp;
}

// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
void QCameraStream_record::stop()
{
  status_t ret = NO_ERROR;
  LOGV("%s: BEGIN", __func__);

  if(!mActive) {
    LOGE("%s : Record stream not started",__func__);
    return;
  }
  mActive =  false;
  Mutex::Autolock lock(mStopCallbackLock);
#if 0 //mzhu, when stop recording, all frame will be dirty. no need to queue frame back to kernel any more
  mRecordFreeQueueLock.lock();
  while(!mRecordFreeQueue.isEmpty()) {
    LOGV("%s : Pre-releasing of Encoder buffers!\n", __FUNCTION__);
    mm_camera_ch_data_buf_t releasedBuf = mRecordFreeQueue.itemAt(0);
    mRecordFreeQueue.removeAt(0);
    mRecordFreeQueueLock.unlock();
    LOGV("%s (%d): releasedBuf.idx = %d\n", __FUNCTION__, __LINE__,
                                              releasedBuf.video.video.idx);
    if(MM_CAMERA_OK != cam_evt_buf_done(mCameraId,&releasedBuf))
        LOGE("%s : Buf Done Failed",__func__);
  }
  mRecordFreeQueueLock.unlock();
#if 0
  while (!mRecordFreeQueue.isEmpty()) {
        LOGE("%s : Waiting for Encoder to release all buffer!\n", __FUNCTION__);
  }
#endif
#endif // mzhu
  /* unregister the notify fn from the mmmm_camera_t object
   *  call stop() in parent class to stop the monitor thread */

  ret = cam_ops_action(mCameraId, FALSE, MM_CAMERA_OPS_VIDEO, 0);
  if (MM_CAMERA_OK != ret) {
    LOGE ("%s ERROR: Video streaming Stop err=%d\n", __func__, ret);
  }

  ret = cam_config_unprepare_buf(mCameraId, MM_CAMERA_CH_VIDEO);
  if(ret != MM_CAMERA_OK){
    LOGE("%s ERROR: Ureg video buf \n", __func__);
  }

  releaseEncodeBuffer();

  mActive = false;
  LOGV("%s: END", __func__);

}
// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
void QCameraStream_record::release()
{
  status_t ret = NO_ERROR;
  LOGV("%s: BEGIN", __func__);

  if(mActive) {
    stop();
  }
  if(!mInit) {
    LOGE("%s : Record stream not initialized",__func__);
    return;
  }

  ret= QCameraStream::deinitChannel(mCameraId, MM_CAMERA_CH_VIDEO);
  if(ret != MM_CAMERA_OK) {
    LOGE("%s:Deinit Video channel failed=%d\n", __func__, ret);
  }
  (void)cam_evt_register_buf_notify(mCameraId, MM_CAMERA_CH_VIDEO,
                                            NULL,
                                            (mm_camera_register_buf_cb_type_t)NULL,
                                            NULL,
                                            NULL);
  mInit = false;
  LOGV("%s: END", __func__);
}

status_t QCameraStream_record::processRecordFrame(void *data)
{
    LOGV("%s : BEGIN",__func__);
    mm_camera_ch_data_buf_t* frame = (mm_camera_ch_data_buf_t*) data;

    Mutex::Autolock lock(mStopCallbackLock);
    if(!mActive) {
      LOGE("Recording Stopped. Returning callback");
      return NO_ERROR;
    }

    if (UNLIKELY(mDebugFps)) {
        debugShowVideoFPS();
    }

    mHalCamCtrl->dumpFrameToFile(frame->video.video.frame, HAL_DUMP_FRM_VIDEO);
    mHalCamCtrl->mCallbackLock.lock();
    camera_data_timestamp_callback rcb = mHalCamCtrl->mDataCbTimestamp;
    void *rdata = mHalCamCtrl->mCallbackCookie;
    mHalCamCtrl->mCallbackLock.unlock();

	nsecs_t timeStamp = nsecs_t(frame->video.video.frame->ts.tv_sec)*1000000000LL + \
                      frame->video.video.frame->ts.tv_nsec;

  LOGE("Send Video frame to services/encoder TimeStamp : %lld",timeStamp);
  mRecordedFrames[frame->video.video.idx] = *frame;

#ifdef USE_ION
  struct ion_flush_data cache_inv_data;
  int ion_fd;
  ion_fd = frame->video.video.frame->ion_dev_fd;
  cache_inv_data.vaddr = (void *)frame->video.video.frame->buffer;
  cache_inv_data.fd = frame->video.video.frame->fd;
  cache_inv_data.handle = frame->video.video.frame->fd_data.handle;
  cache_inv_data.length = frame->video.video.frame->ion_alloc.len;

  if (mHalCamCtrl->cache_ops(ion_fd, &cache_inv_data, ION_IOC_CLEAN_CACHES) < 0)
    LOGE("%s: Cache clean for Video buffer %p fd = %d failed", __func__,
      cache_inv_data.vaddr, cache_inv_data.fd);
#endif

  if (mHalCamCtrl->mStoreMetaDataInFrame) {
    mStopCallbackLock.unlock();
    if(mActive && (rcb != NULL) && (mHalCamCtrl->mMsgEnabled & CAMERA_MSG_VIDEO_FRAME)) {
      rcb(timeStamp, CAMERA_MSG_VIDEO_FRAME,
              mHalCamCtrl->mRecordingMemory.metadata_memory[frame->video.video.idx],
              0, mHalCamCtrl->mCallbackCookie);
    }
  } else {
    mStopCallbackLock.unlock();
    if(mActive && (rcb != NULL) && (mHalCamCtrl->mMsgEnabled & CAMERA_MSG_VIDEO_FRAME)) {
      rcb(timeStamp, CAMERA_MSG_VIDEO_FRAME,
              mHalCamCtrl->mRecordingMemory.camera_memory[frame->video.video.idx],
              0, mHalCamCtrl->mCallbackCookie);
    }
  }

  LOGV("%s : END",__func__);
  return NO_ERROR;
}

//Record Related Functions
status_t QCameraStream_record::initEncodeBuffers()
{
  LOGE("%s : BEGIN",__func__);
  status_t ret = NO_ERROR;
  const char *pmem_region;
  uint32_t frame_len;
  uint8_t num_planes;
  uint32_t planes[VIDEO_MAX_PLANES];
  //cam_ctrl_dimension_t dim;
  int width = 0;  /* width of channel  */
  int height = 0; /* height of channel */
  int buf_cnt;
  pmem_region = "/dev/pmem_adsp";


  memset(&mHalCamCtrl->mRecordingMemory, 0, sizeof(mHalCamCtrl->mRecordingMemory));
  memset(&dim, 0, sizeof(cam_ctrl_dimension_t));
  ret = cam_config_get_parm(mCameraId, MM_CAMERA_PARM_DIMENSION, &dim);
  if (MM_CAMERA_OK != ret) {
    LOGE("%s: ERROR - can't get camera dimension!", __func__);
    return BAD_VALUE;
  }
  else {
    width =  dim.video_width;
    height = dim.video_height;
  }
  num_planes = 2;
  planes[0] = dim.video_frame_offset.mp[0].len;
  planes[1] = dim.video_frame_offset.mp[1].len;
  frame_len = dim.video_frame_offset.frame_len;

  buf_cnt = VIDEO_BUFFER_COUNT;
  if(mHalCamCtrl->isLowPowerCamcorder()) {
    LOGE("%s: lower power camcorder selected", __func__);
    buf_cnt = VIDEO_BUFFER_COUNT_LOW_POWER_CAMCORDER;
  }
    recordframes = new msm_frame[buf_cnt];
    memset(recordframes,0,sizeof(struct msm_frame) * buf_cnt);

		mRecordBuf.video.video.buf.mp = new mm_camera_mp_buf_t[buf_cnt *
                                  sizeof(mm_camera_mp_buf_t)];
		if (!mRecordBuf.video.video.buf.mp) {
			LOGE("%s Error allocating memory for mplanar struct ", __func__);
			return BAD_VALUE;
		}
		memset(mRecordBuf.video.video.buf.mp, 0,
					 buf_cnt * sizeof(mm_camera_mp_buf_t));

    memset(&mHalCamCtrl->mRecordingMemory, 0, sizeof(mHalCamCtrl->mRecordingMemory));
    for (int i=0; i<MM_CAMERA_MAX_NUM_FRAMES;i++) {
        mHalCamCtrl->mRecordingMemory.main_ion_fd[i] = -1;
        mHalCamCtrl->mRecordingMemory.fd[i] = -1;
    }

    mHalCamCtrl->mRecordingMemory.buffer_count = buf_cnt;

		mHalCamCtrl->mRecordingMemory.size = frame_len;
		mHalCamCtrl->mRecordingMemory.cbcr_offset = planes[0];

    for (int cnt = 0; cnt < mHalCamCtrl->mRecordingMemory.buffer_count; cnt++) {
#ifdef USE_ION
      if(mHalCamCtrl->allocate_ion_memory(&mHalCamCtrl->mRecordingMemory, cnt,
        ((0x1 << CAMERA_ION_HEAP_ID) | (0x1 << CAMERA_ION_FALLBACK_HEAP_ID))) < 0) {
        LOGE("%s ION alloc failed\n", __func__);
        return UNKNOWN_ERROR;
      }
#else
		  mHalCamCtrl->mRecordingMemory.fd[cnt] = open("/dev/pmem_adsp", O_RDWR|O_SYNC);
		  if(mHalCamCtrl->mRecordingMemory.fd[cnt] <= 0) {
			  LOGE("%s: no pmem for frame %d", __func__, cnt);
			  return UNKNOWN_ERROR;
		  }
#endif
		  mHalCamCtrl->mRecordingMemory.camera_memory[cnt] =
		    mHalCamCtrl->mGetMemory(mHalCamCtrl->mRecordingMemory.fd[cnt],
		    mHalCamCtrl->mRecordingMemory.size, 1, (void *)this);

      if (mHalCamCtrl->mStoreMetaDataInFrame) {
        mHalCamCtrl->mRecordingMemory.metadata_memory[cnt] =
          mHalCamCtrl->mGetMemory(-1,
          sizeof(struct encoder_media_buffer_type), 1, (void *)this);
        struct encoder_media_buffer_type * packet =
          (struct encoder_media_buffer_type  *)
          mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->data;
        packet->meta_handle = native_handle_create(1, 2); //1 fd, 1 offset and 1 size
        packet->buffer_type = kMetadataBufferTypeCameraSource;
        native_handle_t * nh = const_cast<native_handle_t *>(packet->meta_handle);
        nh->data[0] = mHalCamCtrl->mRecordingMemory.fd[cnt];
        nh->data[1] = 0;
        nh->data[2] = mHalCamCtrl->mRecordingMemory.size;
      }
    	recordframes[cnt].fd = mHalCamCtrl->mRecordingMemory.fd[cnt];
    	recordframes[cnt].buffer = (uint32_t)mHalCamCtrl->mRecordingMemory.camera_memory[cnt]->data;
	    recordframes[cnt].y_off = 0;
	    recordframes[cnt].cbcr_off = mHalCamCtrl->mRecordingMemory.cbcr_offset;
	    recordframes[cnt].path = OUTPUT_TYPE_V;
      recordframes[cnt].fd_data = mHalCamCtrl->mRecordingMemory.ion_info_fd[cnt];
      recordframes[cnt].ion_alloc = mHalCamCtrl->mRecordingMemory.alloc[cnt];
      recordframes[cnt].ion_dev_fd = mHalCamCtrl->mRecordingMemory.main_ion_fd[cnt];

      if (NO_ERROR !=
        mHalCamCtrl->sendMappingBuf(MSM_V4L2_EXT_CAPTURE_MODE_VIDEO, cnt,
        recordframes[cnt].fd, mHalCamCtrl->mRecordingMemory.size, mCameraId,
        CAM_SOCK_MSG_TYPE_FD_MAPPING))
        LOGE("%s: sending mapping data Msg Failed", __func__);

      LOGE ("initRecord :  record heap , video buffers  buffer=%lu fd=%d y_off=%d cbcr_off=%d\n",
		    (unsigned long)recordframes[cnt].buffer, recordframes[cnt].fd, recordframes[cnt].y_off,
		    recordframes[cnt].cbcr_off);
	    //mNumRecordFrames++;

			mRecordBuf.video.video.buf.mp[cnt].frame = recordframes[cnt];
      mRecordBuf.video.video.buf.mp[cnt].frame_offset = 0;
      mRecordBuf.video.video.buf.mp[cnt].num_planes = num_planes;
      /* Plane 0 needs to be set seperately. Set other planes
       * in a loop. */
      mRecordBuf.video.video.buf.mp[cnt].planes[0].reserved[0] =
        mRecordBuf.video.video.buf.mp[cnt].frame_offset;
      mRecordBuf.video.video.buf.mp[cnt].planes[0].length = planes[0];
      mRecordBuf.video.video.buf.mp[cnt].planes[0].m.userptr =
        recordframes[cnt].fd;
      for (int j = 1; j < num_planes; j++) {
        mRecordBuf.video.video.buf.mp[cnt].planes[j].length = planes[j];
        mRecordBuf.video.video.buf.mp[cnt].planes[j].m.userptr =
          recordframes[cnt].fd;
        mRecordBuf.video.video.buf.mp[cnt].planes[j].reserved[0] =
          mRecordBuf.video.video.buf.mp[cnt].planes[j-1].reserved[0] +
          mRecordBuf.video.video.buf.mp[cnt].planes[j-1].length;
      }
    }

    //memset(&mRecordBuf, 0, sizeof(mRecordBuf));
    mRecordBuf.ch_type = MM_CAMERA_CH_VIDEO;
    mRecordBuf.video.video.num = mHalCamCtrl->mRecordingMemory.buffer_count;//kRecordBufferCount;
    //mRecordBuf.video.video.frame_offset = &record_offset[0];
    //mRecordBuf.video.video.frame = &recordframes[0];
    LOGE("%s : END",__func__);
    return NO_ERROR;
}

void QCameraStream_record::releaseRecordingFrame(const void *opaque)
{
    LOGV("%s : BEGIN, opaque = 0x%p",__func__, opaque);
    if(!mActive)
    {
        LOGE("%s : Recording already stopped!!! Leak???",__func__);
        return;
    }
    for(int cnt = 0; cnt < mHalCamCtrl->mRecordingMemory.buffer_count; cnt++) {
      if (mHalCamCtrl->mStoreMetaDataInFrame) {
        if(mHalCamCtrl->mRecordingMemory.metadata_memory[cnt] &&
                mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->data == opaque) {
            /* found the match */
            if(MM_CAMERA_OK != cam_evt_buf_done(mCameraId, &mRecordedFrames[cnt]))
                LOGE("%s : Buf Done Failed",__func__);
            LOGV("%s : END",__func__);
            return;
        }
      } else {
        if(mHalCamCtrl->mRecordingMemory.camera_memory[cnt] &&
                mHalCamCtrl->mRecordingMemory.camera_memory[cnt]->data == opaque) {
            /* found the match */
            if(MM_CAMERA_OK != cam_evt_buf_done(mCameraId, &mRecordedFrames[cnt]))
                LOGE("%s : Buf Done Failed",__func__);
            LOGV("%s : END",__func__);
            return;
        }
      }
    }
	LOGE("%s: cannot find the matched frame with opaue = 0x%p", __func__, opaque);
}

void QCameraStream_record::debugShowVideoFPS() const
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
    LOGI("Video Frames Per Second: %.4f", mFps);
    mLastFpsTime = now;
    mLastFrameCount = mFrameCount;
  }
}

#if 0
sp<IMemoryHeap> QCameraStream_record::getHeap() const
{
  return mRecordHeap != NULL ? mRecordHeap->mHeap : NULL;
}

#endif
status_t  QCameraStream_record::takeLiveSnapshot(){
	return true;
}

}//namespace android

