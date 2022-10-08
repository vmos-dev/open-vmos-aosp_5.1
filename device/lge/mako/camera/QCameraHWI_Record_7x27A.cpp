/*
** Copyright (c) 2012 The Linux Foundation. All rights reserved.
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

#define ALOG_NDEBUG 0
#define ALOG_NIDEBUG 0
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
  ALOGV("%s: BEGIN", __func__);
  QCameraStream* pme = new QCameraStream_record(cameraId, mode);
  ALOGV("%s: END", __func__);
  return pme;
}

// ---------------------------------------------------------------------------
// QCameraStream_record deleteInstance()
// ---------------------------------------------------------------------------
void QCameraStream_record::deleteInstance(QCameraStream *ptr)
{
  ALOGV("%s: BEGIN", __func__);
  if (ptr){
    ptr->release();
    delete ptr;
    ptr = NULL;
  }
  ALOGV("%s: END", __func__);
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
  ALOGV("%s: BEGIN", __func__);

  property_get("persist.debug.sf.showfps", value, "0");
  mDebugFps = atoi(value);

  ALOGV("%s: END", __func__);
}

// ---------------------------------------------------------------------------
// QCameraStream_record Destructor
// ---------------------------------------------------------------------------
QCameraStream_record::~QCameraStream_record() {
  ALOGV("%s: BEGIN", __func__);
  if(mActive) {
    stop();
  }
  if(mInit) {
    release();
  }
  mInit = false;
  mActive = false;
  ALOGV("%s: END", __func__);

}

// ---------------------------------------------------------------------------
// QCameraStream_record Callback from mm_camera
// ---------------------------------------------------------------------------
static void record_notify_cb(mm_camera_ch_data_buf_t *bufs_new,
                              void *user_data)
{
  QCameraStream_record *pme = (QCameraStream_record *)user_data;
  mm_camera_ch_data_buf_t *bufs_used = 0;
  ALOGV("%s: BEGIN", __func__);

  /*
  * Call Function Process Video Data
  */
  pme->processRecordFrame(bufs_new);
  ALOGV("%s: END", __func__);
}

// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
status_t QCameraStream_record::init()
{
  status_t ret = NO_ERROR;
  ALOGV("%s: BEGIN", __func__);
  mInit = true;
  ALOGV("%s: END", __func__);
  return ret;
}
// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------

status_t QCameraStream_record::start()
{
  status_t ret = NO_ERROR;
  ALOGE("%s: BEGIN", __func__);

  ret = initEncodeBuffers();
  if (NO_ERROR!=ret) {
    ALOGE("%s ERROR: Buffer Allocation Failed\n",__func__);
    return ret;
  }
  Mutex::Autolock l(&mHalCamCtrl->mRecordLock);
  mHalCamCtrl->mReleasedRecordingFrame = false;

  mHalCamCtrl->mStartRecording  = true;

  ALOGV("%s: END", __func__);
  return ret;
}

// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
void QCameraStream_record::stop()
{
  status_t ret = NO_ERROR;
  ALOGE("%s: BEGIN", __func__);
  mHalCamCtrl->mStartRecording  = false;
  Mutex::Autolock l(&mHalCamCtrl->mRecordLock);
  {
        mHalCamCtrl->mRecordFrameLock.lock();
        mHalCamCtrl->mReleasedRecordingFrame = true;
        mHalCamCtrl->mRecordWait.signal();
        mHalCamCtrl-> mRecordFrameLock.unlock();
  }

  for(int cnt = 0; cnt < mHalCamCtrl->mPreviewMemory.buffer_count; cnt++) {
    if (mHalCamCtrl->mStoreMetaDataInFrame) {
      struct encoder_media_buffer_type * packet =
          (struct encoder_media_buffer_type  *)
          mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->data;
      native_handle_delete(const_cast<native_handle_t *>(packet->meta_handle));
      mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->release(
		    mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]);
    }
  }
  ALOGV("%s: END", __func__);

}
// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
void QCameraStream_record::release()
{
  status_t ret = NO_ERROR;
  ALOGV("%s: BEGIN", __func__);
  ALOGV("%s: END", __func__);
}

status_t QCameraStream_record::processRecordFrame(void *data)
{
  ALOGE("%s : BEGIN",__func__);
  ALOGE("%s : END",__func__);
  return NO_ERROR;
}

//Record Related Functions
status_t QCameraStream_record::initEncodeBuffers()
{
  ALOGE("%s : BEGIN",__func__);
  status_t ret = NO_ERROR;
    for (int cnt = 0; cnt < mHalCamCtrl->mPreviewMemory.buffer_count; cnt++) {
      if (mHalCamCtrl->mStoreMetaDataInFrame) {
        mHalCamCtrl->mRecordingMemory.metadata_memory[cnt] =
          mHalCamCtrl->mGetMemory(-1,
          sizeof(struct encoder_media_buffer_type), 1, (void *)this);
        struct encoder_media_buffer_type * packet =
          (struct encoder_media_buffer_type  *)
          mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->data;
        packet->meta_handle = native_handle_create(1, 3); //1 fd, 1 offset,1 size and 1 data
        packet->buffer_type = kMetadataBufferTypeCameraSource;
        native_handle_t * nh = const_cast<native_handle_t *>(packet->meta_handle);
        nh->data[0] = mHalCamCtrl->mPreviewMemory.private_buffer_handle[cnt]->fd;
        nh->data[1] = 0;
        nh->data[2] = mHalCamCtrl->mPreviewMemory.private_buffer_handle[cnt]->size;
        nh->data[3] = (uint32_t)mHalCamCtrl->mPreviewMemory.camera_memory[cnt]->data;
      }
    }
    ALOGE("%s : END",__func__);
    return NO_ERROR;
}

void QCameraStream_record::releaseEncodeBuffer() {
  for(int cnt = 0; cnt < mHalCamCtrl->mPreviewMemory.buffer_count; cnt++) {
    if (mHalCamCtrl->mStoreMetaDataInFrame) {
      struct encoder_media_buffer_type * packet =
          (struct encoder_media_buffer_type  *)
          mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->data;
      native_handle_delete(const_cast<native_handle_t *>(packet->meta_handle));
      mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->release(
        mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]);

    }
  }
}

void QCameraStream_record::releaseRecordingFrame(const void *opaque)
{
    Mutex::Autolock rLock(&mHalCamCtrl->mRecordFrameLock);
    mHalCamCtrl->mReleasedRecordingFrame = true;
    mHalCamCtrl->mRecordWait.signal();
    ALOGE("%s, Signaling from-",__func__);
}

void QCameraStream_record::debugShowVideoFPS() const
{

}

status_t  QCameraStream_record::takeLiveSnapshot(){
	return true;
}

}//namespace android
