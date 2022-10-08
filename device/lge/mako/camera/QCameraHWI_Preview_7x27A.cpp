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
#define LOG_TAG "QCameraHWI_Preview"
#include <utils/Log.h>
#include <utils/threads.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "QCameraHAL.h"
#include "QCameraHWI.h"
#include <gralloc_priv.h>
#include <genlock.h>

#define UNLIKELY(exp) __builtin_expect(!!(exp), 0)

/* QCameraHWI_Preview class implementation goes here*/
/* following code implement the preview mode's image capture & display logic of this class*/

namespace android {

// ---------------------------------------------------------------------------
// Preview Callback
// ---------------------------------------------------------------------------
static void preview_notify_cb(mm_camera_ch_data_buf_t *frame,
                                void *user_data)
{
  QCameraStream_preview *pme = (QCameraStream_preview *)user_data;
  mm_camera_ch_data_buf_t *bufs_used = 0;
  ALOGV("%s: E", __func__);
  /* for peview data, there is no queue, so directly use*/
  if(pme==NULL) {
    ALOGE("%s: X : Incorrect cookie",__func__);
    /*Call buf done*/
    return;
  }

  pme->processPreviewFrame(frame);
  ALOGV("%s: X", __func__);
}

status_t QCameraStream_preview::setPreviewWindow(preview_stream_ops_t* window)
{
    status_t retVal = NO_ERROR;
    ALOGE(" %s: E ", __FUNCTION__);
    if( window == NULL) {
        ALOGW(" Setting NULL preview window ");
        /* TODO: Current preview window will be invalidated.
         * Release all the buffers back */
       // relinquishBuffers();
    }
    mDisplayLock.lock();
    mPreviewWindow = window;
    mDisplayLock.unlock();
    ALOGV(" %s : X ", __FUNCTION__ );
    return retVal;
}

status_t QCameraStream_preview::getBufferFromSurface() {
    int err = 0;
    int numMinUndequeuedBufs = 0;
	int format = 0;
	status_t ret = NO_ERROR;

    ALOGI(" %s : E ", __FUNCTION__);

    if( mPreviewWindow == NULL) {
		ALOGE("%s: mPreviewWindow = NULL", __func__);
        return INVALID_OPERATION;
	}
    cam_ctrl_dimension_t dim;

	//mDisplayLock.lock();
    cam_config_get_parm(mCameraId, MM_CAMERA_PARM_DIMENSION,&dim);

	format = mHalCamCtrl->getPreviewFormatInfo().Hal_format;
	if(ret != NO_ERROR) {
        ALOGE("%s: display format %d is not supported", __func__, dim.prev_format);
		goto end;
	}
	numMinUndequeuedBufs = 0;
	if(mPreviewWindow->get_min_undequeued_buffer_count) {
    err = mPreviewWindow->get_min_undequeued_buffer_count(mPreviewWindow, &numMinUndequeuedBufs);
		if (err != 0) {
			 ALOGE("get_min_undequeued_buffer_count  failed: %s (%d)",
						strerror(-err), -err);
			 ret = UNKNOWN_ERROR;
			 goto end;
		}
	}
    mHalCamCtrl->mPreviewMemoryLock.lock();
    mHalCamCtrl->mPreviewMemory.buffer_count = kPreviewBufferCount + numMinUndequeuedBufs;;
    err = mPreviewWindow->set_buffer_count(mPreviewWindow, mHalCamCtrl->mPreviewMemory.buffer_count );
    if (err != 0) {
         ALOGE("set_buffer_count failed: %s (%d)",
                    strerror(-err), -err);
         ret = UNKNOWN_ERROR;
		 goto end;
    }
    err = mPreviewWindow->set_buffers_geometry(mPreviewWindow,
                dim.display_width, dim.display_height, format);
    if (err != 0) {
         ALOGE("set_buffers_geometry failed: %s (%d)",
                    strerror(-err), -err);
         ret = UNKNOWN_ERROR;
		 goto end;
    }
    err = mPreviewWindow->set_usage(mPreviewWindow,
                GRALLOC_USAGE_PRIVATE_ADSP_HEAP |
                GRALLOC_USAGE_PRIVATE_UNCACHED);
	if(err != 0) {
        /* set_usage error out */
		ALOGE("%s: set_usage rc = %d", __func__, err);
		ret = UNKNOWN_ERROR;
		goto end;
	}
	for (int cnt = 0; cnt < mHalCamCtrl->mPreviewMemory.buffer_count; cnt++) {
		int stride;
		err = mPreviewWindow->dequeue_buffer(mPreviewWindow,
										&mHalCamCtrl->mPreviewMemory.buffer_handle[cnt],
										&mHalCamCtrl->mPreviewMemory.stride[cnt]);
		if(!err) {
                    err = mPreviewWindow->lock_buffer(this->mPreviewWindow,
                                       mHalCamCtrl->mPreviewMemory.buffer_handle[cnt]);

                    // lock the buffer using genlock
                    ALOGD("%s: camera call genlock_lock", __FUNCTION__);
                    if (GENLOCK_NO_ERROR != genlock_lock_buffer((native_handle_t *)(*mHalCamCtrl->mPreviewMemory.buffer_handle[cnt]),
                                                      GENLOCK_WRITE_LOCK, GENLOCK_MAX_TIMEOUT)) {
                       ALOGE("%s: genlock_lock_buffer(WRITE) failed", __FUNCTION__);
                       mHalCamCtrl->mPreviewMemory.local_flag[cnt] = BUFFER_UNLOCKED;
	               mHalCamCtrl->mPreviewMemoryLock.unlock();
                       return -EINVAL;
                   }
		   mHalCamCtrl->mPreviewMemory.local_flag[cnt] = BUFFER_LOCKED;
		} else
			ALOGE("%s: dequeue_buffer idx = %d err = %d", __func__, cnt, err);

		ALOGE("%s: dequeue buf: %u\n", __func__, (unsigned int)mHalCamCtrl->mPreviewMemory.buffer_handle[cnt]);

		if(err != 0) {
            ALOGE("%s: dequeue_buffer failed: %s (%d)", __func__,
                    strerror(-err), -err);
            ret = UNKNOWN_ERROR;
			for(int i = 0; i < cnt; i++) {
                        ALOGD("%s: camera call genlock_unlock", __FUNCTION__);
                        if (BUFFER_LOCKED == mHalCamCtrl->mPreviewMemory.local_flag[i]) {
                             if (GENLOCK_FAILURE == genlock_unlock_buffer((native_handle_t *)
                                                          (*(mHalCamCtrl->mPreviewMemory.buffer_handle[i])))) {
                                ALOGE("%s: genlock_unlock_buffer failed", __FUNCTION__);
	                        mHalCamCtrl->mPreviewMemoryLock.unlock();
                                return -EINVAL;
                             }
                        }
		        err = mPreviewWindow->cancel_buffer(mPreviewWindow,
										mHalCamCtrl->mPreviewMemory.buffer_handle[i]);
				mHalCamCtrl->mPreviewMemory.buffer_handle[i] = NULL;
				mHalCamCtrl->mPreviewMemory.local_flag[i] = BUFFER_UNLOCKED;
			}
			goto end;
		}
		mHalCamCtrl->mPreviewMemory.private_buffer_handle[cnt] =
		    (struct private_handle_t *)(*mHalCamCtrl->mPreviewMemory.buffer_handle[cnt]);
		mHalCamCtrl->mPreviewMemory.camera_memory[cnt] =
		    mHalCamCtrl->mGetMemory(mHalCamCtrl->mPreviewMemory.private_buffer_handle[cnt]->fd,
			mHalCamCtrl->mPreviewMemory.private_buffer_handle[cnt]->size, 1, (void *)this);
		ALOGE("%s: idx = %d, fd = %d, size = %d, offset = %d", __func__,
            cnt, mHalCamCtrl->mPreviewMemory.private_buffer_handle[cnt]->fd,
			mHalCamCtrl->mPreviewMemory.private_buffer_handle[cnt]->size,
			mHalCamCtrl->mPreviewMemory.private_buffer_handle[cnt]->offset);
	}


	memset(&mHalCamCtrl->mMetadata, 0, sizeof(mHalCamCtrl->mMetadata));
	memset(mHalCamCtrl->mFace, 0, sizeof(mHalCamCtrl->mFace));

    ALOGI(" %s : X ",__FUNCTION__);
end:
	//mDisplayLock.unlock();
	mHalCamCtrl->mPreviewMemoryLock.unlock();

    return NO_ERROR;
}

status_t QCameraStream_preview::putBufferToSurface() {
    int err = 0;
	status_t ret = NO_ERROR;

    ALOGI(" %s : E ", __FUNCTION__);

    //mDisplayLock.lock();
    mHalCamCtrl->mPreviewMemoryLock.lock();
	for (int cnt = 0; cnt < mHalCamCtrl->mPreviewMemory.buffer_count; cnt++) {
        mHalCamCtrl->mPreviewMemory.camera_memory[cnt]->release(mHalCamCtrl->mPreviewMemory.camera_memory[cnt]);
            if (BUFFER_LOCKED == mHalCamCtrl->mPreviewMemory.local_flag[cnt]) {
                ALOGD("%s: camera call genlock_unlock", __FUNCTION__);
	        if (GENLOCK_FAILURE == genlock_unlock_buffer((native_handle_t *)
                                                    (*(mHalCamCtrl->mPreviewMemory.buffer_handle[cnt])))) {
                    ALOGE("%s: genlock_unlock_buffer failed", __FUNCTION__);
	            mHalCamCtrl->mPreviewMemoryLock.unlock();
                    return -EINVAL;
                } else {
                    mHalCamCtrl->mPreviewMemory.local_flag[cnt] = BUFFER_UNLOCKED;
                }
            }
            err = mPreviewWindow->cancel_buffer(mPreviewWindow, mHalCamCtrl->mPreviewMemory.buffer_handle[cnt]);
		ALOGE(" put buffer %d successfully", cnt);
	}
	memset(&mHalCamCtrl->mPreviewMemory, 0, sizeof(mHalCamCtrl->mPreviewMemory));
	mHalCamCtrl->mPreviewMemoryLock.unlock();
	//mDisplayLock.unlock();
    ALOGI(" %s : X ",__FUNCTION__);
    return NO_ERROR;
}

void QCameraStream_preview::notifyROIEvent(fd_roi_t roi)
{
    switch (roi.type) {
    case FD_ROI_TYPE_HEADER:
        {
            mDisplayLock.lock();
            mNumFDRcvd = 0;
            memset(mHalCamCtrl->mFace, 0, sizeof(mHalCamCtrl->mFace));
            mHalCamCtrl->mMetadata.faces = mHalCamCtrl->mFace;
            mHalCamCtrl->mMetadata.number_of_faces = roi.d.hdr.num_face_detected;
            if(mHalCamCtrl->mMetadata.number_of_faces > MAX_ROI)
              mHalCamCtrl->mMetadata.number_of_faces = MAX_ROI;
            mDisplayLock.unlock();

            if (mHalCamCtrl->mMetadata.number_of_faces == 0) {
                // Clear previous faces
                mHalCamCtrl->mCallbackLock.lock();
                camera_data_callback pcb = mHalCamCtrl->mDataCb;
                mHalCamCtrl->mCallbackLock.unlock();

                if (pcb && (mHalCamCtrl->mMsgEnabled & CAMERA_MSG_PREVIEW_METADATA)){
                    ALOGE("%s: Face detection RIO callback", __func__);
                    pcb(CAMERA_MSG_PREVIEW_METADATA, NULL, 0, &mHalCamCtrl->mMetadata, mHalCamCtrl->mCallbackCookie);
                }
            }
        }
        break;
    case FD_ROI_TYPE_DATA:
        {
            mDisplayLock.lock();
            int idx = roi.d.data.idx;
            if (idx >= mHalCamCtrl->mMetadata.number_of_faces) {
                mDisplayLock.unlock();
                ALOGE("%s: idx %d out of boundary %d", __func__, idx, mHalCamCtrl->mMetadata.number_of_faces);
                break;
            }

            mHalCamCtrl->mFace[idx].id = roi.d.data.face.id;
            mHalCamCtrl->mFace[idx].score = roi.d.data.face.score / 10; // keep within range 0~100

            // top
            mHalCamCtrl->mFace[idx].rect[0] =
               roi.d.data.face.face_boundary.x*2000/mHalCamCtrl->mDimension.display_width - 1000;
            //right
            mHalCamCtrl->mFace[idx].rect[1] =
               roi.d.data.face.face_boundary.y*2000/mHalCamCtrl->mDimension.display_height - 1000;
            //bottom
            mHalCamCtrl->mFace[idx].rect[2] =  mHalCamCtrl->mFace[idx].rect[0] +
               roi.d.data.face.face_boundary.dx*2000/mHalCamCtrl->mDimension.display_width;
            //left
            mHalCamCtrl->mFace[idx].rect[3] = mHalCamCtrl->mFace[idx].rect[1] +
               roi.d.data.face.face_boundary.dy*2000/mHalCamCtrl->mDimension.display_height;

            // Center of left eye
            mHalCamCtrl->mFace[idx].left_eye[0] =
              roi.d.data.face.left_eye_center[0]*2000/mHalCamCtrl->mDimension.display_width - 1000;
            mHalCamCtrl->mFace[idx].left_eye[1] =
              roi.d.data.face.left_eye_center[1]*2000/mHalCamCtrl->mDimension.display_height - 1000;

            // Center of right eye
            mHalCamCtrl->mFace[idx].right_eye[0] =
              roi.d.data.face.right_eye_center[0]*2000/mHalCamCtrl->mDimension.display_width - 1000;
            mHalCamCtrl->mFace[idx].right_eye[1] =
              roi.d.data.face.right_eye_center[1]*2000/mHalCamCtrl->mDimension.display_height - 1000;

            // Center of mouth
            mHalCamCtrl->mFace[idx].mouth[0] =
              roi.d.data.face.mouth_center[0]*2000/mHalCamCtrl->mDimension.display_width - 1000;
            mHalCamCtrl->mFace[idx].mouth[1] =
              roi.d.data.face.mouth_center[1]*2000/mHalCamCtrl->mDimension.display_height - 1000;

            mHalCamCtrl->mFace[idx].smile_degree = roi.d.data.face.smile_degree;
            mHalCamCtrl->mFace[idx].smile_score = roi.d.data.face.smile_confidence;
            mHalCamCtrl->mFace[idx].blink_detected = roi.d.data.face.blink_detected;
            mHalCamCtrl->mFace[idx].face_recognised = roi.d.data.face.is_face_recognised;
            mHalCamCtrl->mFace[idx].gaze_angle = roi.d.data.face.gaze_angle;

            /* newly added */
            // upscale by 2 to recover from demaen downscaling
            mHalCamCtrl->mFace[idx].updown_dir = roi.d.data.face.updown_dir*2;
            mHalCamCtrl->mFace[idx].leftright_dir = roi.d.data.face.leftright_dir*2;
            mHalCamCtrl->mFace[idx].roll_dir = roi.d.data.face.roll_dir*2;

            mHalCamCtrl->mFace[idx].leye_blink = roi.d.data.face.left_blink;
            mHalCamCtrl->mFace[idx].reye_blink = roi.d.data.face.right_blink;
            mHalCamCtrl->mFace[idx].left_right_gaze = roi.d.data.face.left_right_gaze;
            mHalCamCtrl->mFace[idx].top_bottom_gaze = roi.d.data.face.top_bottom_gaze;

            ALOGE("%s: Face(%d, %d, %d, %d), leftEye(%d, %d), rightEye(%d, %d), mouth(%d, %d), smile(%d, %d), blinked(%d)", __func__,
               mHalCamCtrl->mFace[idx].rect[0],  mHalCamCtrl->mFace[idx].rect[1],
               mHalCamCtrl->mFace[idx].rect[2],  mHalCamCtrl->mFace[idx].rect[3],
               mHalCamCtrl->mFace[idx].left_eye[0], mHalCamCtrl->mFace[idx].left_eye[1],
               mHalCamCtrl->mFace[idx].right_eye[0], mHalCamCtrl->mFace[idx].right_eye[1],
               mHalCamCtrl->mFace[idx].mouth[0], mHalCamCtrl->mFace[idx].mouth[1],
               roi.d.data.face.smile_degree, roi.d.data.face.smile_confidence, roi.d.data.face.blink_detected);

             mNumFDRcvd++;
             mDisplayLock.unlock();

             if (mNumFDRcvd == mHalCamCtrl->mMetadata.number_of_faces) {
                 mHalCamCtrl->mCallbackLock.lock();
                 camera_data_callback pcb = mHalCamCtrl->mDataCb;
                 mHalCamCtrl->mCallbackLock.unlock();

                 if (pcb && (mHalCamCtrl->mMsgEnabled & CAMERA_MSG_PREVIEW_METADATA)){
                     ALOGE("%s: Face detection RIO callback with %d faces detected (score=%d)", __func__, mNumFDRcvd, mHalCamCtrl->mFace[idx].score);
                     pcb(CAMERA_MSG_PREVIEW_METADATA, NULL, 0, &mHalCamCtrl->mMetadata, mHalCamCtrl->mCallbackCookie);
                 }
             }
        }
        break;
    }
}

status_t QCameraStream_preview::initDisplayBuffers()
{
    status_t ret = NO_ERROR;
    int width = 0;  /* width of channel  */
    int height = 0; /* height of channel */
    uint32_t frame_len = 0; /* frame planner length */
    int buffer_num = 4; /* number of buffers for display */
    const char *pmem_region;
    uint8_t num_planes = 0;
    uint32_t planes[VIDEO_MAX_PLANES];

    cam_ctrl_dimension_t dim;

    ALOGE("%s:BEGIN",__func__);
	memset(&mHalCamCtrl->mMetadata, 0, sizeof(camera_frame_metadata_t));
	mHalCamCtrl->mPreviewMemoryLock.lock();
	memset(&mHalCamCtrl->mPreviewMemory, 0, sizeof(mHalCamCtrl->mPreviewMemory));
	mHalCamCtrl->mPreviewMemoryLock.unlock();
	memset(&mNotifyBuffer, 0, sizeof(mNotifyBuffer));

  /* get preview size, by qury mm_camera*/
    memset(&dim, 0, sizeof(cam_ctrl_dimension_t));

    memset(&(this->mDisplayStreamBuf),0, sizeof(this->mDisplayStreamBuf));

    ret = cam_config_get_parm(mCameraId, MM_CAMERA_PARM_DIMENSION, &dim);
    if (MM_CAMERA_OK != ret) {
      ALOGE("%s: error - can't get camera dimension!", __func__);
      ALOGE("%s: X", __func__);
      return BAD_VALUE;
    }else {
      width =  dim.display_width,
      height = dim.display_height;
    }

	ret = getBufferFromSurface();
	if(ret != NO_ERROR) {
	 ALOGE("%s: cannot get memory from surface texture client, ret = %d", __func__, ret);
	 return ret;
	}

  /* set 4 buffers for display */
  memset(&mDisplayStreamBuf, 0, sizeof(mDisplayStreamBuf));
  mHalCamCtrl->mPreviewMemoryLock.lock();
  this->mDisplayStreamBuf.num = mHalCamCtrl->mPreviewMemory.buffer_count;
  this->myMode=myMode; /*Need to assign this in constructor after translating from mask*/
  num_planes = 2;
  planes[0] = dim.display_frame_offset.mp[0].len;
  planes[1] = dim.display_frame_offset.mp[1].len;
  this->mDisplayStreamBuf.frame_len = dim.display_frame_offset.frame_len;

  mDisplayBuf.preview.buf.mp = new mm_camera_mp_buf_t[mDisplayStreamBuf.num];
  if (!mDisplayBuf.preview.buf.mp) {
	  ALOGE("%s Error allocating memory for mplanar struct ", __func__);
  }
  memset(mDisplayBuf.preview.buf.mp, 0,
    mDisplayStreamBuf.num * sizeof(mm_camera_mp_buf_t));

  /*allocate memory for the buffers*/
  void *vaddr = NULL;
  for(int i = 0; i < mDisplayStreamBuf.num; i++){
	  if (mHalCamCtrl->mPreviewMemory.private_buffer_handle[i] == NULL)
		  continue;
      mDisplayStreamBuf.frame[i].fd = mHalCamCtrl->mPreviewMemory.private_buffer_handle[i]->fd;
      mDisplayStreamBuf.frame[i].cbcr_off = planes[0];
      mDisplayStreamBuf.frame[i].y_off = 0;
      mDisplayStreamBuf.frame[i].path = OUTPUT_TYPE_P;
	  mHalCamCtrl->mPreviewMemory.addr_offset[i] =
	      mHalCamCtrl->mPreviewMemory.private_buffer_handle[i]->offset;
      mDisplayStreamBuf.frame[i].buffer =
          (long unsigned int)mHalCamCtrl->mPreviewMemory.camera_memory[i]->data;

	  ALOGE("%s: idx = %d, fd = %d, size = %d, cbcr_offset = %d, y_offset = %d, offset = %d, vaddr = 0x%x",
		  __func__, i,
		  mDisplayStreamBuf.frame[i].fd,
		  mHalCamCtrl->mPreviewMemory.private_buffer_handle[i]->size,
		  mDisplayStreamBuf.frame[i].cbcr_off,
		  mDisplayStreamBuf.frame[i].y_off,
		  mHalCamCtrl->mPreviewMemory.addr_offset[i],
		  (uint32_t)mDisplayStreamBuf.frame[i].buffer);


        mDisplayBuf.preview.buf.mp[i].frame = mDisplayStreamBuf.frame[i];
        mDisplayBuf.preview.buf.mp[i].frame_offset = mHalCamCtrl->mPreviewMemory.addr_offset[i];
        mDisplayBuf.preview.buf.mp[i].num_planes = num_planes;

		/* Plane 0 needs to be set seperately. Set other planes
         * in a loop. */
        mDisplayBuf.preview.buf.mp[i].planes[0].length = planes[0];
        mDisplayBuf.preview.buf.mp[i].planes[0].m.userptr = mDisplayStreamBuf.frame[i].fd;
        mDisplayBuf.preview.buf.mp[i].planes[0].data_offset = 0;
        mDisplayBuf.preview.buf.mp[i].planes[0].reserved[0] =
          mDisplayBuf.preview.buf.mp[i].frame_offset;
        for (int j = 1; j < num_planes; j++) {
          mDisplayBuf.preview.buf.mp[i].planes[j].length = planes[j];
          mDisplayBuf.preview.buf.mp[i].planes[j].m.userptr =
            mDisplayStreamBuf.frame[i].fd;
		  mDisplayBuf.preview.buf.mp[i].planes[j].data_offset = 0;
          mDisplayBuf.preview.buf.mp[i].planes[j].reserved[0] =
            mDisplayBuf.preview.buf.mp[i].planes[j-1].reserved[0] +
            mDisplayBuf.preview.buf.mp[i].planes[j-1].length;
        }

		for (int j = 0; j < num_planes; j++) {
			ALOGE("Planes: %d length: %d userptr: %lu offset: %d\n",
				 j, mDisplayBuf.preview.buf.mp[i].planes[j].length,
				 mDisplayBuf.preview.buf.mp[i].planes[j].m.userptr,
				 mDisplayBuf.preview.buf.mp[i].planes[j].reserved[0]);
		}

  }/*end of for loop*/

 /* register the streaming buffers for the channel*/
  mDisplayBuf.ch_type = MM_CAMERA_CH_PREVIEW;
  mDisplayBuf.preview.num = mDisplayStreamBuf.num;
  mHalCamCtrl->mPreviewMemoryLock.unlock();
  ALOGE("%s:END",__func__);
  return NO_ERROR;

end:
  if (MM_CAMERA_OK == ret ) {
    ALOGV("%s: X - NO_ERROR ", __func__);
    return NO_ERROR;
  }

    ALOGV("%s: out of memory clean up", __func__);
  /* release the allocated memory */

  ALOGV("%s: X - BAD_VALUE ", __func__);
  return BAD_VALUE;
}

void QCameraStream_preview::dumpFrameToFile(struct msm_frame* newFrame)
{
  int32_t enabled = 0;
  int frm_num;
  uint32_t  skip_mode;
  char value[PROPERTY_VALUE_MAX];
  char buf[32];
  int w, h;
  static int count = 0;
  cam_ctrl_dimension_t dim;
  int file_fd;
  int rc = 0;
  int len;
  unsigned long addr;
  unsigned long * tmp = (unsigned long *)newFrame->buffer;
  addr = *tmp;
  status_t ret = cam_config_get_parm(mHalCamCtrl->mCameraId,
                 MM_CAMERA_PARM_DIMENSION, &dim);

  w = dim.display_width;
  h = dim.display_height;
  len = (w * h)*3/2;
  count++;
  if(count < 100) {
    snprintf(buf, sizeof(buf), "/data/mzhu%d.yuv", count);
    file_fd = open(buf, O_RDWR | O_CREAT, 0777);

    rc = write(file_fd, (const void *)addr, len);
    ALOGE("%s: file='%s', vaddr_old=0x%x, addr_map = 0x%p, len = %d, rc = %d",
          __func__, buf, (uint32_t)newFrame->buffer, (void *)addr, len, rc);
    close(file_fd);
    ALOGE("%s: dump %s, rc = %d, len = %d", __func__, buf, rc, len);
  }
}

status_t QCameraStream_preview::processPreviewFrame(mm_camera_ch_data_buf_t *frame)
{
  ALOGV("%s",__func__);
  int err = 0;
  int msgType = 0;
  camera_memory_t *data = NULL;
  camera_frame_metadata_t *metadata = NULL;

  Mutex::Autolock lock(mStopCallbackLock);
  if(!mActive) {
    ALOGE("Preview Stopped. Returning callback");
    return NO_ERROR;
  }
  if(mHalCamCtrl==NULL) {
    ALOGE("%s: X: HAL control object not set",__func__);
    /*Call buf done*/
    return BAD_VALUE;
  }

  mHalCamCtrl->mCallbackLock.lock();
  camera_data_timestamp_callback rcb = mHalCamCtrl->mDataCbTimestamp;
  void *rdata = mHalCamCtrl->mCallbackCookie;
  mHalCamCtrl->mCallbackLock.unlock();

  if (UNLIKELY(mHalCamCtrl->mDebugFps)) {
      mHalCamCtrl->debugShowPreviewFPS();
  }
  //dumpFrameToFile(frame->def.frame);
  mHalCamCtrl->dumpFrameToFile(frame->def.frame, HAL_DUMP_FRM_PREVIEW);

  nsecs_t timeStamp = systemTime();

  mHalCamCtrl->mPreviewMemoryLock.lock();
  mNotifyBuffer[frame->def.idx] = *frame;
  // mzhu fix me, need to check meta data also.

  ALOGI("Enqueue buf handle %p\n",
  mHalCamCtrl->mPreviewMemory.buffer_handle[frame->def.idx]);
  ALOGD("%s: camera call genlock_unlock", __FUNCTION__);
    if (BUFFER_LOCKED == mHalCamCtrl->mPreviewMemory.local_flag[frame->def.idx]) {
        if (GENLOCK_FAILURE == genlock_unlock_buffer((native_handle_t*)
	            (*mHalCamCtrl->mPreviewMemory.buffer_handle[frame->def.idx]))) {
            ALOGE("%s: genlock_unlock_buffer failed", __FUNCTION__);
	    mHalCamCtrl->mPreviewMemoryLock.unlock();
            return -EINVAL;
        } else {
            mHalCamCtrl->mPreviewMemory.local_flag[frame->def.idx] = BUFFER_UNLOCKED;
        }
    } else {
        ALOGE("%s: buffer to be enqueued is not locked", __FUNCTION__);
	mHalCamCtrl->mPreviewMemoryLock.unlock();
        return -EINVAL;
    }
  err = this->mPreviewWindow->enqueue_buffer(this->mPreviewWindow,
    (buffer_handle_t *)mHalCamCtrl->mPreviewMemory.buffer_handle[frame->def.idx]);
  if(err != 0) {
    ALOGE("%s: enqueue_buffer failed, err = %d", __func__, err);
  }
  buffer_handle_t *buffer_handle = NULL;
  int tmp_stride = 0;
  err = this->mPreviewWindow->dequeue_buffer(this->mPreviewWindow,
              &buffer_handle, &tmp_stride);
  if (err == NO_ERROR && buffer_handle != NULL) {
      err = this->mPreviewWindow->lock_buffer(this->mPreviewWindow, buffer_handle);
      ALOGD("%s: camera call genlock_lock", __FUNCTION__);
      if (GENLOCK_FAILURE == genlock_lock_buffer((native_handle_t*)(*buffer_handle), GENLOCK_WRITE_LOCK,
                                                 GENLOCK_MAX_TIMEOUT)) {
            ALOGE("%s: genlock_lock_buffer(WRITE) failed", __FUNCTION__);
	    mHalCamCtrl->mPreviewMemoryLock.unlock();
            return -EINVAL;
      }
      for(int i = 0; i < mHalCamCtrl->mPreviewMemory.buffer_count; i++) {
		  ALOGD("h1: %p h2: %p\n", mHalCamCtrl->mPreviewMemory.buffer_handle[i], buffer_handle);
		  if(mHalCamCtrl->mPreviewMemory.buffer_handle[i] == buffer_handle) {
	          mm_camera_ch_data_buf_t tmp_frame;
                  mHalCamCtrl->mPreviewMemory.local_flag[i] = BUFFER_LOCKED;
              if(MM_CAMERA_OK != cam_evt_buf_done(mCameraId, &mNotifyBuffer[i])) {
                  ALOGD("BUF DONE FAILED");
                  mHalCamCtrl->mPreviewMemoryLock.unlock();
                  return BAD_VALUE;
              }
			  break;
		  }
	  }
  } else
      ALOGE("%s: error in dequeue_buffer, enqueue_buffer idx = %d, no free buffer now", __func__, frame->def.idx);
  /* Save the last displayed frame. We'll be using it to fill the gap between
     when preview stops and postview start during snapshot.*/
  mLastQueuedFrame = &(mDisplayStreamBuf.frame[frame->def.idx]);
  mHalCamCtrl->mPreviewMemoryLock.unlock();

  mHalCamCtrl->mCallbackLock.lock();
  camera_data_callback pcb = mHalCamCtrl->mDataCb;
  mHalCamCtrl->mCallbackLock.unlock();
  ALOGD("Message enabled = 0x%x", mHalCamCtrl->mMsgEnabled);

  if (pcb != NULL) {
      //Sending preview callback if corresponding Msgs are enabled
      if(mHalCamCtrl->mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME) {
          msgType |=  CAMERA_MSG_PREVIEW_FRAME;
          data = mHalCamCtrl->mPreviewMemory.camera_memory[frame->def.idx];//mPreviewHeap->mBuffers[frame->def.idx];
      } else {
          data = NULL;
      }
      if(msgType) {
          mStopCallbackLock.unlock();
          pcb(msgType, data, 0, metadata, mHalCamCtrl->mCallbackCookie);
      }
	  ALOGD("end of cb");
  }
  if(rcb != NULL)
  {
    if (mHalCamCtrl->mStoreMetaDataInFrame)
    {
          mStopCallbackLock.unlock();
          if(mHalCamCtrl->mStartRecording == true &&( mHalCamCtrl->mMsgEnabled & CAMERA_MSG_VIDEO_FRAME))
          rcb(timeStamp, CAMERA_MSG_VIDEO_FRAME,
              mHalCamCtrl->mRecordingMemory.metadata_memory[frame->def.idx],
              0, mHalCamCtrl->mCallbackCookie);
    }
    else
    {
        if(mHalCamCtrl->mStartRecording == true &&( mHalCamCtrl->mMsgEnabled & CAMERA_MSG_VIDEO_FRAME))
        {
            mStopCallbackLock.unlock();
            rcb(timeStamp, CAMERA_MSG_VIDEO_FRAME,
              mHalCamCtrl->mPreviewMemory.camera_memory[frame->def.idx],
              0, mHalCamCtrl->mCallbackCookie);
        }
    }
  }

  /* Save the last displayed frame. We'll be using it to fill the gap between
     when preview stops and postview start during snapshot.*/
  //mLastQueuedFrame = frame->def.frame;
/*
  if(MM_CAMERA_OK != cam_evt_buf_done(mCameraId, frame))
  {
      ALOGE("BUF DONE FAILED");
      return BAD_VALUE;
  }
*/
  return NO_ERROR;
}

// ---------------------------------------------------------------------------
// QCameraStream_preview
// ---------------------------------------------------------------------------

QCameraStream_preview::
QCameraStream_preview(int cameraId, camera_mode_t mode)
  : QCameraStream(cameraId,mode),
    mLastQueuedFrame(NULL),
    mNumFDRcvd(0)
  {
    mHalCamCtrl = NULL;
    ALOGE("%s: E", __func__);
    ALOGE("%s: X", __func__);
  }
// ---------------------------------------------------------------------------
// QCameraStream_preview
// ---------------------------------------------------------------------------

QCameraStream_preview::~QCameraStream_preview() {
    ALOGV("%s: E", __func__);
	if(mActive) {
		stop();
	}
	if(mInit) {
		release();
	}
	mInit = false;
	mActive = false;
    ALOGV("%s: X", __func__);

}
// ---------------------------------------------------------------------------
// QCameraStream_preview
// ---------------------------------------------------------------------------

status_t QCameraStream_preview::init() {

  status_t ret = NO_ERROR;
  ALOGV("%s: E", __func__);

  ret = QCameraStream::initChannel (mCameraId, MM_CAMERA_CH_PREVIEW_MASK);
  if (NO_ERROR!=ret) {
    ALOGE("%s E: can't init native cammera preview ch\n",__func__);
    return ret;
  }

  ALOGE("Debug : %s : initChannel",__func__);
  /* register a notify into the mmmm_camera_t object*/
  (void) cam_evt_register_buf_notify(mCameraId, MM_CAMERA_CH_PREVIEW,
                                     preview_notify_cb, MM_CAMERA_REG_BUF_CB_INFINITE, 0, this);
  ALOGE("Debug : %s : cam_evt_register_buf_notify",__func__);
  buffer_handle_t *buffer_handle = NULL;
  int tmp_stride = 0;
  mInit = true;
  return ret;
}
// ---------------------------------------------------------------------------
// QCameraStream_preview
// ---------------------------------------------------------------------------

status_t QCameraStream_preview::start()
{
    ALOGV("%s: E", __func__);
    status_t ret = NO_ERROR;
    mm_camera_reg_buf_t *reg_buf=&mDisplayBuf;

    Mutex::Autolock lock(mStopCallbackLock);

    /* call start() in parent class to start the monitor thread*/
    //QCameraStream::start ();
    setFormat(MM_CAMERA_CH_PREVIEW_MASK);

    if(NO_ERROR!=initDisplayBuffers()){
        return BAD_VALUE;
    }
    ALOGE("Debug : %s : initDisplayBuffers",__func__);
    ret = cam_config_prepare_buf(mCameraId, reg_buf);
    ALOGE("Debug : %s : cam_config_prepare_buf",__func__);
    if(ret != MM_CAMERA_OK) {
        ALOGV("%s:reg preview buf err=%d\n", __func__, ret);
        ret = BAD_VALUE;
    }else
        ret = NO_ERROR;

	/* For preview, the OP_MODE we set is dependent upon whether we are
       starting camera or camcorder. For snapshot, anyway we disable preview.
       However, for ZSL we need to set OP_MODE to OP_MODE_ZSL and not
       OP_MODE_VIDEO. We'll set that for now in CamCtrl. So in case of
       ZSL we skip setting Mode here */

    if (!(myMode & CAMERA_ZSL_MODE)) {
        ALOGE("Setting OP MODE to MM_CAMERA_OP_MODE_VIDEO");
        mm_camera_op_mode_type_t op_mode=MM_CAMERA_OP_MODE_VIDEO;
        ret = cam_config_set_parm (mCameraId, MM_CAMERA_PARM_OP_MODE,
                                        &op_mode);
        ALOGE("OP Mode Set");

        if(MM_CAMERA_OK != ret) {
          ALOGE("%s: X :set mode MM_CAMERA_OP_MODE_VIDEO err=%d\n", __func__, ret);
          return BAD_VALUE;
        }
    }else {
        ALOGE("Setting OP MODE to MM_CAMERA_OP_MODE_ZSL");
        mm_camera_op_mode_type_t op_mode=MM_CAMERA_OP_MODE_ZSL;
        ret = cam_config_set_parm (mCameraId, MM_CAMERA_PARM_OP_MODE,
                                        &op_mode);
        if(MM_CAMERA_OK != ret) {
          ALOGE("%s: X :set mode MM_CAMERA_OP_MODE_ZSL err=%d\n", __func__, ret);
          return BAD_VALUE;
        }
     }

    /* call mm_camera action start(...)  */
    ALOGE("Starting Preview/Video Stream. ");
    ret = cam_ops_action(mCameraId, TRUE, MM_CAMERA_OPS_PREVIEW, 0);

    if (MM_CAMERA_OK != ret) {
      ALOGE ("%s: preview streaming start err=%d\n", __func__, ret);
      return BAD_VALUE;
    }

    ALOGE("Debug : %s : Preview streaming Started",__func__);
    ret = NO_ERROR;

    mActive =  true;
    ALOGE("%s: X", __func__);
    return NO_ERROR;
  }


// ---------------------------------------------------------------------------
// QCameraStream_preview
// ---------------------------------------------------------------------------
  void QCameraStream_preview::stop() {
    ALOGE("%s: E", __func__);
    int ret=MM_CAMERA_OK;

    if(!mActive) {
      return;
    }
    mActive =  false;
    Mutex::Autolock lock(mStopCallbackLock);
    /* unregister the notify fn from the mmmm_camera_t object*/

    /* call stop() in parent class to stop the monitor thread*/
    ret = cam_ops_action(mCameraId, FALSE, MM_CAMERA_OPS_PREVIEW, 0);
    if(MM_CAMERA_OK != ret) {
      ALOGE ("%s: camera preview stop err=%d\n", __func__, ret);
    }
    ALOGE("Debug : %s : Preview streaming Stopped",__func__);
    ret = cam_config_unprepare_buf(mCameraId, MM_CAMERA_CH_PREVIEW);
    if(ret != MM_CAMERA_OK) {
      ALOGE("%s:Unreg preview buf err=%d\n", __func__, ret);
      //ret = BAD_VALUE;
    }

    ALOGE("Debug : %s : Buffer Unprepared",__func__);
    if (mDisplayBuf.preview.buf.mp != NULL) {
        delete[] mDisplayBuf.preview.buf.mp;
    }
	/*free camera_memory handles and return buffer back to surface*/
    putBufferToSurface();

    ALOGE("%s: X", __func__);

  }
// ---------------------------------------------------------------------------
// QCameraStream_preview
// ---------------------------------------------------------------------------
  void QCameraStream_preview::release() {

    ALOGE("%s : BEGIN",__func__);
    int ret=MM_CAMERA_OK,i;

    if(!mInit)
    {
      ALOGE("%s : Stream not Initalized",__func__);
      return;
    }

    if(mActive) {
      this->stop();
    }

    ret= QCameraStream::deinitChannel(mCameraId, MM_CAMERA_CH_PREVIEW);
    ALOGE("Debug : %s : De init Channel",__func__);
    if(ret != MM_CAMERA_OK) {
      ALOGE("%s:Deinit preview channel failed=%d\n", __func__, ret);
      //ret = BAD_VALUE;
    }

    (void)cam_evt_register_buf_notify(mCameraId, MM_CAMERA_CH_PREVIEW,
                                      NULL,
					(mm_camera_register_buf_cb_type_t)NULL,
					NULL,
					NULL);
	mInit = false;
    ALOGE("%s: END", __func__);

  }

QCameraStream*
QCameraStream_preview::createInstance(int cameraId,
                                      camera_mode_t mode)
{
  QCameraStream* pme = new QCameraStream_preview(cameraId, mode);
  return pme;
}
// ---------------------------------------------------------------------------
// QCameraStream_preview
// ---------------------------------------------------------------------------

void QCameraStream_preview::deleteInstance(QCameraStream *p)
{
  if (p){
    ALOGV("%s: BEGIN", __func__);
    p->release();
    delete p;
    p = NULL;
    ALOGV("%s: END", __func__);
  }
}


/* Temp helper function */
void *QCameraStream_preview::getLastQueuedFrame(void)
{
    return mLastQueuedFrame;
}

status_t QCameraStream_preview::initPreviewOnlyBuffers()
{
  /*1. for 7x27a, this shall not called;
    2. this file shall be removed ASAP
    so put a dummy function to just pass the compile*/
  return INVALID_OPERATION;
}

// ---------------------------------------------------------------------------
// No code beyone this line
// ---------------------------------------------------------------------------
}; // namespace android
