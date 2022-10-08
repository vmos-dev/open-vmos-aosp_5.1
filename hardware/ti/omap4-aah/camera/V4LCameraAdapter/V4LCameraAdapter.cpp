/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
* @file V4LCameraAdapter.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/


#include "V4LCameraAdapter.h"
#include "CameraHal.h"
#include "TICameraParameters.h"
#include "DebugUtils.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev.h>

#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>

#include <cutils/properties.h>
#define UNLIKELY( exp ) (__builtin_expect( (exp) != 0, false ))
static int mDebugFps = 0;

#define Q16_OFFSET 16

#define HERE(Msg) {CAMHAL_LOGEB("--=== %s===--\n", Msg);}

namespace Ti {
namespace Camera {

//frames skipped before recalculating the framerate
#define FPS_PERIOD 30

//define this macro to save first few raw frames when starting the preview.
//#define SAVE_RAW_FRAMES 1
//#define DUMP_CAPTURE_FRAME 1
//#define PPM_PER_FRAME_CONVERSION 1

//Proto Types
static void convertYUV422i_yuyvTouyvy(uint8_t *src, uint8_t *dest, size_t size );
static void convertYUV422ToNV12Tiler(unsigned char *src, unsigned char *dest, int width, int height );
static void convertYUV422ToNV12(unsigned char *src, unsigned char *dest, int width, int height );

android::Mutex gV4LAdapterLock;
char device[15];


/*--------------------Camera Adapter Class STARTS here-----------------------------*/

/*--------------------V4L wrapper functions -------------------------------*/
status_t V4LCameraAdapter::v4lIoctl (int fd, int req, void* argp) {
    status_t ret = NO_ERROR;
    errno = 0;

    do {
        ret = ioctl (fd, req, argp);
    }while (-1 == ret && EINTR == errno);

    return ret;
}

status_t V4LCameraAdapter::v4lInitMmap(int& count) {
    status_t ret = NO_ERROR;

    //First allocate adapter internal buffers at V4L level for USB Cam
    //These are the buffers from which we will copy the data into overlay buffers
    /* Check if camera can handle NB_BUFFER buffers */
    mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->rb.memory = V4L2_MEMORY_MMAP;
    mVideoInfo->rb.count = count;

    ret = v4lIoctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
    if (ret < 0) {
        CAMHAL_LOGEB("VIDIOC_REQBUFS failed: %s", strerror(errno));
        return ret;
    }

    count = mVideoInfo->rb.count;
    for (int i = 0; i < count; i++) {

        memset (&mVideoInfo->buf, 0, sizeof (struct v4l2_buffer));

        mVideoInfo->buf.index = i;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        ret = v4lIoctl (mCameraHandle, VIDIOC_QUERYBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEB("Unable to query buffer (%s)", strerror(errno));
            return ret;
        }

        mVideoInfo->mem[i] = mmap (NULL,
               mVideoInfo->buf.length,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               mCameraHandle,
               mVideoInfo->buf.m.offset);

        CAMHAL_LOGVB(" mVideoInfo->mem[%d]=%p ; mVideoInfo->buf.length = %d", i, mVideoInfo->mem[i], mVideoInfo->buf.length);
        if (mVideoInfo->mem[i] == MAP_FAILED) {
            CAMHAL_LOGEB("Unable to map buffer [%d]. (%s)", i, strerror(errno));
            return -1;
        }
    }
    return ret;
}

status_t V4LCameraAdapter::v4lInitUsrPtr(int& count) {
    status_t ret = NO_ERROR;

    mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->rb.memory = V4L2_MEMORY_USERPTR;
    mVideoInfo->rb.count = count;

    ret = v4lIoctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
    if (ret < 0) {
        CAMHAL_LOGEB("VIDIOC_REQBUFS failed for USERPTR: %s", strerror(errno));
        return ret;
    }

    count = mVideoInfo->rb.count;
    return ret;
}

status_t V4LCameraAdapter::v4lStartStreaming () {
    status_t ret = NO_ERROR;
    enum v4l2_buf_type bufType;

    if (!mVideoInfo->isStreaming) {
        bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        ret = v4lIoctl (mCameraHandle, VIDIOC_STREAMON, &bufType);
        if (ret < 0) {
            CAMHAL_LOGEB("StartStreaming: Unable to start capture: %s", strerror(errno));
            return ret;
        }
        mVideoInfo->isStreaming = true;
    }
    return ret;
}

status_t V4LCameraAdapter::v4lStopStreaming (int nBufferCount) {
    status_t ret = NO_ERROR;
    enum v4l2_buf_type bufType;

    if (mVideoInfo->isStreaming) {
        bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        ret = v4lIoctl (mCameraHandle, VIDIOC_STREAMOFF, &bufType);
        if (ret < 0) {
            CAMHAL_LOGEB("StopStreaming: Unable to stop capture: %s", strerror(errno));
            goto EXIT;
        }
        mVideoInfo->isStreaming = false;

        /* Unmap buffers */
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;
        for (int i = 0; i < nBufferCount; i++) {
            if (munmap(mVideoInfo->mem[i], mVideoInfo->buf.length) < 0) {
                CAMHAL_LOGEA("munmap() failed");
            }
        }

        //free the memory allocated during REQBUFS, by setting the count=0
        mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->rb.memory = V4L2_MEMORY_MMAP;
        mVideoInfo->rb.count = 0;

        ret = v4lIoctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
        if (ret < 0) {
            CAMHAL_LOGEB("VIDIOC_REQBUFS failed: %s", strerror(errno));
            goto EXIT;
        }
    }
EXIT:
    return ret;
}

status_t V4LCameraAdapter::v4lSetFormat (int width, int height, uint32_t pix_format) {
    status_t ret = NO_ERROR;

    mVideoInfo->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = v4lIoctl(mCameraHandle, VIDIOC_G_FMT, &mVideoInfo->format);
    if (ret < 0) {
        CAMHAL_LOGEB("VIDIOC_G_FMT Failed: %s", strerror(errno));
    }

    mVideoInfo->width = width;
    mVideoInfo->height = height;
    mVideoInfo->framesizeIn = (width * height << 1);
    mVideoInfo->formatIn = DEFAULT_PIXEL_FORMAT;

    mVideoInfo->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->format.fmt.pix.width = width;
    mVideoInfo->format.fmt.pix.height = height;
    mVideoInfo->format.fmt.pix.pixelformat = pix_format;

    ret = v4lIoctl(mCameraHandle, VIDIOC_S_FMT, &mVideoInfo->format);
    if (ret < 0) {
        CAMHAL_LOGEB("VIDIOC_S_FMT Failed: %s", strerror(errno));
        return ret;
    }
    v4lIoctl(mCameraHandle, VIDIOC_G_FMT, &mVideoInfo->format);
    CAMHAL_LOGDB("VIDIOC_G_FMT : WxH = %dx%d", mVideoInfo->format.fmt.pix.width, mVideoInfo->format.fmt.pix.height);
    return ret;
}

status_t V4LCameraAdapter::restartPreview ()
{
    status_t ret = NO_ERROR;
    int width = 0;
    int height = 0;
    struct v4l2_streamparm streamParams;

    //configure for preview size and pixel format.
    mParams.getPreviewSize(&width, &height);

    ret = v4lSetFormat (width, height, DEFAULT_PIXEL_FORMAT);
    if (ret < 0) {
        CAMHAL_LOGEB("v4lSetFormat Failed: %s", strerror(errno));
        goto EXIT;
    }

    ret = v4lInitMmap(mPreviewBufferCount);
    if (ret < 0) {
        CAMHAL_LOGEB("v4lInitMmap Failed: %s", strerror(errno));
        goto EXIT;
    }

    //set frame rate
    streamParams.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamParams.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
    streamParams.parm.capture.capturemode = V4L2_MODE_HIGHQUALITY;
    streamParams.parm.capture.timeperframe.denominator = FPS_PERIOD;
    streamParams.parm.capture.timeperframe.numerator= 1;
    ret = v4lIoctl(mCameraHandle, VIDIOC_S_PARM, &streamParams);
    if (ret < 0) {
        CAMHAL_LOGEB("VIDIOC_S_PARM Failed: %s", strerror(errno));
        goto EXIT;
    }

    for (int i = 0; i < mPreviewBufferCountQueueable; i++) {

        mVideoInfo->buf.index = i;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        ret = v4lIoctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEA("VIDIOC_QBUF Failed");
            goto EXIT;
        }
        nQueued++;
    }

    ret = v4lStartStreaming();
    CAMHAL_LOGDA("Ready for preview....");
EXIT:
    return ret;
}

/*--------------------Camera Adapter Functions-----------------------------*/
status_t V4LCameraAdapter::initialize(CameraProperties::Properties* caps)
{
    char value[PROPERTY_VALUE_MAX];

    LOG_FUNCTION_NAME;
    property_get("debug.camera.showfps", value, "0");
    mDebugFps = atoi(value);

    int ret = NO_ERROR;

    // Allocate memory for video info structure
    mVideoInfo = (struct VideoInfo *) calloc (1, sizeof (struct VideoInfo));
    if(!mVideoInfo) {
        ret = NO_MEMORY;
        goto EXIT;
    }

    if ((mCameraHandle = open(device, O_RDWR | O_NONBLOCK) ) == -1) {
        CAMHAL_LOGEB("Error while opening handle to V4L2 Camera: %s", strerror(errno));
        ret = BAD_VALUE;
        goto EXIT;
    }

    ret = v4lIoctl (mCameraHandle, VIDIOC_QUERYCAP, &mVideoInfo->cap);
    if (ret < 0) {
        CAMHAL_LOGEA("Error when querying the capabilities of the V4L Camera");
        ret = BAD_VALUE;
        goto EXIT;
    }

    if ((mVideoInfo->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
        CAMHAL_LOGEA("Error while adapter initialization: video capture not supported.");
        ret = BAD_VALUE;
        goto EXIT;
    }

    if (!(mVideoInfo->cap.capabilities & V4L2_CAP_STREAMING)) {
        CAMHAL_LOGEA("Error while adapter initialization: Capture device does not support streaming i/o");
        ret = BAD_VALUE;
        goto EXIT;
    }

    // Initialize flags
    mPreviewing = false;
    mVideoInfo->isStreaming = false;
    mRecording = false;
    mCapturing = false;
EXIT:
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::fillThisBuffer(CameraBuffer *frameBuf, CameraFrame::FrameType frameType)
{
    status_t ret = NO_ERROR;
    int idx = 0;
    LOG_FUNCTION_NAME;

    if ( frameType == CameraFrame::IMAGE_FRAME) { //(1 > mCapturedFrames)
        // Signal end of image capture
        if ( NULL != mEndImageCaptureCallback) {
            CAMHAL_LOGDB("===========Signal End Image Capture==========");
            mEndImageCaptureCallback(mEndCaptureData);
        }
        goto EXIT;
    }
    if ( !mVideoInfo->isStreaming ) {
        goto EXIT;
    }

    idx = mPreviewBufs.valueFor(frameBuf);
    if(idx < 0) {
        CAMHAL_LOGEB("Wrong index  = %d",idx);
        goto EXIT;
    }

    mVideoInfo->buf.index = idx;
    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

    ret = v4lIoctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
    if (ret < 0) {
       CAMHAL_LOGEA("VIDIOC_QBUF Failed");
       goto EXIT;
    }
     nQueued++;
EXIT:
    LOG_FUNCTION_NAME_EXIT;
    return ret;

}

status_t V4LCameraAdapter::setParameters(const android::CameraParameters &params)
{
    status_t ret = NO_ERROR;
    int width, height;
    struct v4l2_streamparm streamParams;

    LOG_FUNCTION_NAME;

    if(!mPreviewing && !mCapturing) {
        params.getPreviewSize(&width, &height);
        CAMHAL_LOGDB("Width * Height %d x %d format 0x%x", width, height, DEFAULT_PIXEL_FORMAT);

        ret = v4lSetFormat( width, height, DEFAULT_PIXEL_FORMAT);
        if (ret < 0) {
            CAMHAL_LOGEB(" VIDIOC_S_FMT Failed: %s", strerror(errno));
            goto EXIT;
        }
        //set frame rate
        // Now its fixed to 30 FPS
        streamParams.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        streamParams.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
        streamParams.parm.capture.capturemode = V4L2_MODE_HIGHQUALITY;
        streamParams.parm.capture.timeperframe.denominator = FPS_PERIOD;
        streamParams.parm.capture.timeperframe.numerator= 1;
        ret = v4lIoctl(mCameraHandle, VIDIOC_S_PARM, &streamParams);
        if (ret < 0) {
            CAMHAL_LOGEB(" VIDIOC_S_PARM Failed: %s", strerror(errno));
            goto EXIT;
        }
        int actualFps = streamParams.parm.capture.timeperframe.denominator / streamParams.parm.capture.timeperframe.numerator;
        CAMHAL_LOGDB("Actual FPS set is : %d.", actualFps);
    }

    // Udpate the current parameter set
    mParams = params;

EXIT:
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}


void V4LCameraAdapter::getParameters(android::CameraParameters& params)
{
    LOG_FUNCTION_NAME;

    // Return the current parameter set
    params = mParams;

    LOG_FUNCTION_NAME_EXIT;
}


///API to give the buffers to Adapter
status_t V4LCameraAdapter::useBuffers(CameraMode mode, CameraBuffer *bufArr, int num, size_t length, unsigned int queueable)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mLock);

    switch(mode)
        {
        case CAMERA_PREVIEW:
            mPreviewBufferCountQueueable = queueable;
            ret = UseBuffersPreview(bufArr, num);
            break;

        case CAMERA_IMAGE_CAPTURE:
            mCaptureBufferCountQueueable = queueable;
            ret = UseBuffersCapture(bufArr, num);
            break;

        case CAMERA_VIDEO:
            //@warn Video capture is not fully supported yet
            mPreviewBufferCountQueueable = queueable;
            ret = UseBuffersPreview(bufArr, num);
            break;

        case CAMERA_MEASUREMENT:
            break;

        default:
            break;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t V4LCameraAdapter::UseBuffersCapture(CameraBuffer *bufArr, int num) {
    int ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    if(NULL == bufArr) {
        ret = BAD_VALUE;
        goto EXIT;
    }

    for (int i = 0; i < num; i++) {
        //Associate each Camera internal buffer with the one from Overlay
        mCaptureBufs.add(&bufArr[i], i);
        CAMHAL_LOGDB("capture- buff [%d] = 0x%x ",i, mCaptureBufs.keyAt(i));
    }

    mCaptureBuffersAvailable.clear();
    for (int i = 0; i < mCaptureBufferCountQueueable; i++ ) {
        mCaptureBuffersAvailable.add(&mCaptureBuffers[i], 0);
    }

    // initial ref count for undeqeueued buffers is 1 since buffer provider
    // is still holding on to it
    for (int i = mCaptureBufferCountQueueable; i < num; i++ ) {
        mCaptureBuffersAvailable.add(&mCaptureBuffers[i], 1);
    }

    // Update the preview buffer count
    mCaptureBufferCount = num;
EXIT:
    LOG_FUNCTION_NAME_EXIT;
    return ret;

}

status_t V4LCameraAdapter::UseBuffersPreview(CameraBuffer *bufArr, int num)
{
    int ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    if(NULL == bufArr) {
        ret = BAD_VALUE;
        goto EXIT;
    }

    ret = v4lInitMmap(num);
    if (ret == NO_ERROR) {
        for (int i = 0; i < num; i++) {
            //Associate each Camera internal buffer with the one from Overlay
            mPreviewBufs.add(&bufArr[i], i);
            CAMHAL_LOGDB("Preview- buff [%d] = 0x%x ",i, mPreviewBufs.keyAt(i));
        }

        // Update the preview buffer count
        mPreviewBufferCount = num;
    }
EXIT:
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::takePicture() {
    status_t ret = NO_ERROR;
    int width = 0;
    int height = 0;
    size_t yuv422i_buff_size = 0;
    int index = 0;
    char *fp = NULL;
    CameraBuffer *buffer = NULL;
    CameraFrame frame;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mCaptureBufsLock);

    if(mCapturing) {
        CAMHAL_LOGEA("Already Capture in Progress...");
        ret = BAD_VALUE;
        goto EXIT;
    }

    mCapturing = true;
    mPreviewing = false;

    // Stop preview streaming
    ret = v4lStopStreaming(mPreviewBufferCount);
    if (ret < 0 ) {
        CAMHAL_LOGEB("v4lStopStreaming Failed: %s", strerror(errno));
        goto EXIT;
    }

    //configure for capture image size and pixel format.
    mParams.getPictureSize(&width, &height);
    CAMHAL_LOGDB("Image Capture Size WxH = %dx%d",width,height);
    yuv422i_buff_size = width * height * 2;

    ret = v4lSetFormat (width, height, DEFAULT_PIXEL_FORMAT);
    if (ret < 0) {
        CAMHAL_LOGEB("v4lSetFormat Failed: %s", strerror(errno));
        goto EXIT;
    }

    ret = v4lInitMmap(mCaptureBufferCount);
    if (ret < 0) {
        CAMHAL_LOGEB("v4lInitMmap Failed: %s", strerror(errno));
        goto EXIT;
    }

    for (int i = 0; i < mCaptureBufferCountQueueable; i++) {

       mVideoInfo->buf.index = i;
       mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
       mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

       ret = v4lIoctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
       if (ret < 0) {
           CAMHAL_LOGEA("VIDIOC_QBUF Failed");
           ret = BAD_VALUE;
           goto EXIT;
       }
       nQueued++;
    }

    ret = v4lStartStreaming();
    if (ret < 0) {
        CAMHAL_LOGEB("v4lStartStreaming Failed: %s", strerror(errno));
        goto EXIT;
    }

    CAMHAL_LOGDA("Streaming started for Image Capture");

    //get the frame and send to encode as JPG
    fp = this->GetFrame(index);
    if(!fp) {
        CAMHAL_LOGEA("!!! Captured frame is NULL !!!!");
        ret = BAD_VALUE;
        goto EXIT;
    }

    CAMHAL_LOGDA("::Capture Frame received from V4L::");
    buffer = mCaptureBufs.keyAt(index);
    CAMHAL_LOGVB("## captureBuf[%d] = 0x%x, yuv422i_buff_size=%d", index, buffer->opaque, yuv422i_buff_size);

    //copy the yuv422i data to the image buffer.
    memcpy(buffer->opaque, fp, yuv422i_buff_size);

#ifdef DUMP_CAPTURE_FRAME
    //dump the YUV422 buffer in to a file
    //a folder should have been created at /data/misc/camera/raw/
    {
        int fd =-1;
        fd = open("/data/misc/camera/raw/captured_yuv422i_dump.yuv", O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
        if(fd < 0) {
            CAMHAL_LOGEB("Unable to open file: %s",  strerror(fd));
        }
        else {
            write(fd, fp, yuv422i_buff_size );
            close(fd);
            CAMHAL_LOGDB("::Captured Frame dumped at /data/misc/camera/raw/captured_yuv422i_dump.yuv::");
        }
    }
#endif

    CAMHAL_LOGDA("::sending capture frame to encoder::");
    frame.mFrameType = CameraFrame::IMAGE_FRAME;
    frame.mBuffer = buffer;
    frame.mLength = yuv422i_buff_size;
    frame.mWidth = width;
    frame.mHeight = height;
    frame.mAlignment = width*2;
    frame.mOffset = 0;
    frame.mTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
    frame.mFrameMask = (unsigned int)CameraFrame::IMAGE_FRAME;
    frame.mQuirks |= CameraFrame::ENCODE_RAW_YUV422I_TO_JPEG;
    frame.mQuirks |= CameraFrame::FORMAT_YUV422I_YUYV;

    ret = setInitFrameRefCount(frame.mBuffer, frame.mFrameMask);
    if (ret != NO_ERROR) {
        CAMHAL_LOGDB("Error in setInitFrameRefCount %d", ret);
    } else {
        ret = sendFrameToSubscribers(&frame);
    }

    // Stop streaming after image capture
    ret = v4lStopStreaming(mCaptureBufferCount);
    if (ret < 0 ) {
        CAMHAL_LOGEB("v4lStopStreaming Failed: %s", strerror(errno));
        goto EXIT;
    }

    ret = restartPreview();
EXIT:
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::stopImageCapture()
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    //Release image buffers
    if ( NULL != mReleaseImageBuffersCallback ) {
        mReleaseImageBuffersCallback(mReleaseData);
    }
    mCaptureBufs.clear();

    mCapturing = false;
    mPreviewing = true;
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::autoFocus()
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    //autoFocus is not implemented. Just return.
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::startPreview()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    android::AutoMutex lock(mPreviewBufsLock);

    if(mPreviewing) {
        ret = BAD_VALUE;
        goto EXIT;
    }

    for (int i = 0; i < mPreviewBufferCountQueueable; i++) {

        mVideoInfo->buf.index = i;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        ret = v4lIoctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEA("VIDIOC_QBUF Failed");
            goto EXIT;
        }
        nQueued++;
    }

    ret = v4lStartStreaming();

    // Create and start preview thread for receiving buffers from V4L Camera
    if(!mCapturing) {
        mPreviewThread = new PreviewThread(this);
        CAMHAL_LOGDA("Created preview thread");
    }

    //Update the flag to indicate we are previewing
    mPreviewing = true;
    mCapturing = false;

EXIT:
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::stopPreview()
{
    enum v4l2_buf_type bufType;
    int ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    android::AutoMutex lock(mStopPreviewLock);

    if(!mPreviewing) {
        return NO_INIT;
    }
    mPreviewing = false;

    ret = v4lStopStreaming(mPreviewBufferCount);
    if (ret < 0) {
        CAMHAL_LOGEB("StopStreaming: FAILED: %s", strerror(errno));
    }

    nQueued = 0;
    nDequeued = 0;
    mFramesWithEncoder = 0;

    mPreviewBufs.clear();

    mPreviewThread->requestExitAndWait();
    mPreviewThread.clear();

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

char * V4LCameraAdapter::GetFrame(int &index)
{
    int ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

    /* DQ */
    // Some V4L drivers, notably uvc, protect each incoming call with
    // a driver-wide mutex.  If we use poll() or blocking VIDIOC_DQBUF ioctl
    // here then we sometimes would run into a deadlock on VIDIO_QBUF ioctl.
    while(true) {
      if(!mVideoInfo->isStreaming) {
        return NULL;
      }

      ret = v4lIoctl(mCameraHandle, VIDIOC_DQBUF, &mVideoInfo->buf);
      if((ret == 0) || (errno != EAGAIN)) {
        break;
      }
    }

    if (ret < 0) {
        CAMHAL_LOGEA("GetFrame: VIDIOC_DQBUF Failed");
        return NULL;
    }
    nDequeued++;

    index = mVideoInfo->buf.index;

    LOG_FUNCTION_NAME_EXIT;
    return (char *)mVideoInfo->mem[mVideoInfo->buf.index];
}

//API to get the frame size required to be allocated. This size is used to override the size passed
//by camera service when VSTAB/VNF is turned ON for example
status_t V4LCameraAdapter::getFrameSize(size_t &width, size_t &height)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    // Just return the current preview size, nothing more to do here.
    mParams.getPreviewSize(( int * ) &width,
                           ( int * ) &height);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t V4LCameraAdapter::getFrameDataSize(size_t &dataFrameSize, size_t bufferCount)
{
    // We don't support meta data, so simply return
    return NO_ERROR;
}

status_t V4LCameraAdapter::getPictureBufferSize(CameraFrame &frame, size_t bufferCount)
{
    int width = 0;
    int height = 0;
    int bytesPerPixel = 2; // for YUV422i; default pixel format

    LOG_FUNCTION_NAME;

    mParams.getPictureSize( &width, &height );
    frame.mLength = width * height * bytesPerPixel;
    frame.mWidth = width;
    frame.mHeight = height;
    frame.mAlignment = width * bytesPerPixel;

    CAMHAL_LOGDB("Picture size: W x H = %u x %u (size=%u bytes, alignment=%u bytes)",
                 frame.mWidth, frame.mHeight, frame.mLength, frame.mAlignment);
    LOG_FUNCTION_NAME_EXIT;
    return NO_ERROR;
}

static void debugShowFPS()
{
    static int mFrameCount = 0;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    if(mDebugFps) {
        mFrameCount++;
        if (!(mFrameCount & 0x1F)) {
            nsecs_t now = systemTime();
            nsecs_t diff = now - mLastFpsTime;
            mFps = ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
            mLastFpsTime = now;
            mLastFrameCount = mFrameCount;
            CAMHAL_LOGI("Camera %d Frames, %f FPS", mFrameCount, mFps);
        }
    }
}

status_t V4LCameraAdapter::recalculateFPS()
{
    float currentFPS;

    mFrameCount++;

    if ( ( mFrameCount % FPS_PERIOD ) == 0 )
        {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFPSTime;
        currentFPS =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFPSTime = now;
        mLastFrameCount = mFrameCount;

        if ( 1 == mIter )
            {
            mFPS = currentFPS;
            }
        else
            {
            //cumulative moving average
            mFPS = mLastFPS + (currentFPS - mLastFPS)/mIter;
            }

        mLastFPS = mFPS;
        mIter++;
        }

    return NO_ERROR;
}

void V4LCameraAdapter::onOrientationEvent(uint32_t orientation, uint32_t tilt)
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;
}


V4LCameraAdapter::V4LCameraAdapter(size_t sensor_index)
{
    LOG_FUNCTION_NAME;

    // Nothing useful to do in the constructor
    mFramesWithEncoder = 0;

    LOG_FUNCTION_NAME_EXIT;
}

V4LCameraAdapter::~V4LCameraAdapter()
{
    LOG_FUNCTION_NAME;

    // Close the camera handle and free the video info structure
    close(mCameraHandle);

    if (mVideoInfo)
      {
        free(mVideoInfo);
        mVideoInfo = NULL;
      }

    LOG_FUNCTION_NAME_EXIT;
}

static void convertYUV422i_yuyvTouyvy(uint8_t *src, uint8_t *dest, size_t size ) {
    //convert YUV422I yuyv to uyvy format.
    uint32_t *bf = (uint32_t*)src;
    uint32_t *dst = (uint32_t*)dest;

    LOG_FUNCTION_NAME;

    if (!src || !dest) {
        return;
    }

    for(size_t i = 0; i < size; i = i+4)
    {
        dst[0] = ((bf[0] & 0x00FF00FF) << 8) | ((bf[0] & 0xFF00FF00) >> 8);
        bf++;
        dst++;
    }

    LOG_FUNCTION_NAME_EXIT;
}

static void convertYUV422ToNV12Tiler(unsigned char *src, unsigned char *dest, int width, int height ) {
    //convert YUV422I to YUV420 NV12 format and copies directly to preview buffers (Tiler memory).
    int stride = 4096;
    unsigned char *bf = src;
    unsigned char *dst_y = dest;
    unsigned char *dst_uv = dest + ( height * stride);
#ifdef PPM_PER_FRAME_CONVERSION
    static int frameCount = 0;
    static nsecs_t ppm_diff = 0;
    nsecs_t ppm_start  = systemTime();
#endif

    LOG_FUNCTION_NAME;

    if (width % 16 ) {
        for(int i = 0; i < height; i++) {
            for(int j = 0; j < width; j++) {
                *dst_y = *bf;
                dst_y++;
                bf = bf + 2;
            }
            dst_y += (stride - width);
        }

        bf = src;
        bf++;  //UV sample
        for(int i = 0; i < height/2; i++) {
            for(int j=0; j<width; j++) {
                *dst_uv = *bf;
                dst_uv++;
                bf = bf + 2;
            }
            bf = bf + width*2;
            dst_uv = dst_uv + (stride - width);
        }
    } else {
        //neon conversion
        for(int i = 0; i < height; i++) {
            int n = width;
            int skip = i & 0x1;       // skip uv elements for the odd rows
            asm volatile (
                "   pld [%[src], %[src_stride], lsl #2]                         \n\t"
                "   cmp %[n], #16                                               \n\t"
                "   blt 5f                                                      \n\t"
                "0: @ 16 pixel copy                                             \n\t"
                "   vld2.8  {q0, q1} , [%[src]]! @ q0 = yyyy.. q1 = uvuv..      \n\t"
                "                                @ now q0 = y q1 = uv           \n\t"
                "   vst1.32   {d0,d1}, [%[dst_y]]!                              \n\t"
                "   cmp    %[skip], #0                                          \n\t"
                "   bne 1f                                                      \n\t"
                "   vst1.32  {d2,d3},[%[dst_uv]]!                               \n\t"
                "1: @ skip odd rows for UV                                      \n\t"
                "   sub %[n], %[n], #16                                         \n\t"
                "   cmp %[n], #16                                               \n\t"
                "   bge 0b                                                      \n\t"
                "5: @ end                                                       \n\t"
#ifdef NEEDS_ARM_ERRATA_754319_754320
                "   vmov s0,s0  @ add noop for errata item                      \n\t"
#endif
                : [dst_y] "+r" (dst_y), [dst_uv] "+r" (dst_uv), [src] "+r" (src), [n] "+r" (n)
                : [src_stride] "r" (width), [skip] "r" (skip)
                : "cc", "memory", "q0", "q1", "q2", "d0", "d1", "d2", "d3"
            );
            dst_y = dst_y + (stride - width);
            if (skip == 0) {
                dst_uv = dst_uv + (stride - width);
            }
        } //end of for()
    }

#ifdef PPM_PER_FRAME_CONVERSION
    ppm_diff += (systemTime() - ppm_start);
    frameCount++;

    if (frameCount >= 30) {
        ppm_diff = ppm_diff / frameCount;
        LOGD("PPM: YUV422i to NV12 Conversion(%d x %d): %llu us ( %llu ms )", width, height,
                ns2us(ppm_diff), ns2ms(ppm_diff) );
        ppm_diff = 0;
        frameCount = 0;
    }
#endif

    LOG_FUNCTION_NAME_EXIT;
}

static void convertYUV422ToNV12(unsigned char *src, unsigned char *dest, int width, int height ) {
    //convert YUV422I to YUV420 NV12 format.
    unsigned char *bf = src;
    unsigned char *dst_y = dest;
    unsigned char *dst_uv = dest + (width * height);

    LOG_FUNCTION_NAME;

    if (width % 16 ) {
        for(int i = 0; i < height; i++) {
            for(int j = 0; j < width; j++) {
                *dst_y = *bf;
                dst_y++;
                bf = bf + 2;
            }
        }

        bf = src;
        bf++;  //UV sample
        for(int i = 0; i < height/2; i++) {
            for(int j=0; j<width; j++) {
                *dst_uv = *bf;
                dst_uv++;
                bf = bf + 2;
            }
            bf = bf + width*2;
        }
    } else {
        //neon conversion
        for(int i = 0; i < height; i++) {
            int n = width;
            int skip = i & 0x1;       // skip uv elements for the odd rows
            asm volatile (
                "   pld [%[src], %[src_stride], lsl #2]                         \n\t"
                "   cmp %[n], #16                                               \n\t"
                "   blt 5f                                                      \n\t"
                "0: @ 16 pixel copy                                             \n\t"
                "   vld2.8  {q0, q1} , [%[src]]! @ q0 = yyyy.. q1 = uvuv..      \n\t"
                "                                @ now q0 = y q1 = uv           \n\t"
                "   vst1.32   {d0,d1}, [%[dst_y]]!                              \n\t"
                "   cmp    %[skip], #0                                          \n\t"
                "   bne 1f                                                      \n\t"
                "   vst1.32  {d2,d3},[%[dst_uv]]!                               \n\t"
                "1: @ skip odd rows for UV                                      \n\t"
                "   sub %[n], %[n], #16                                         \n\t"
                "   cmp %[n], #16                                               \n\t"
                "   bge 0b                                                      \n\t"
                "5: @ end                                                       \n\t"
#ifdef NEEDS_ARM_ERRATA_754319_754320
                "   vmov s0,s0  @ add noop for errata item                      \n\t"
#endif
                : [dst_y] "+r" (dst_y), [dst_uv] "+r" (dst_uv), [src] "+r" (src), [n] "+r" (n)
                : [src_stride] "r" (width), [skip] "r" (skip)
                : "cc", "memory", "q0", "q1", "q2", "d0", "d1", "d2", "d3"
            );
        }
    }

    LOG_FUNCTION_NAME_EXIT;
}

#ifdef SAVE_RAW_FRAMES
void saveFile(unsigned char* buff, int buff_size) {
    static int      counter = 1;
    int             fd = -1;
    char            fn[256];

    LOG_FUNCTION_NAME;
    if (counter > 3) {
        return;
    }
    //dump nv12 buffer
    counter++;
    sprintf(fn, "/data/misc/camera/raw/nv12_dump_%03d.yuv", counter);
    CAMHAL_LOGEB("Dumping nv12 frame to a file : %s.", fn);

    fd = open(fn, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
    if(fd < 0) {
        CAMHAL_LOGE("Unable to open file %s: %s", fn, strerror(fd));
        return;
    }

    write(fd, buff, buff_size );
    close(fd);

    LOG_FUNCTION_NAME_EXIT;
}
#endif

/* Preview Thread */
// ---------------------------------------------------------------------------

int V4LCameraAdapter::previewThread()
{
    status_t ret = NO_ERROR;
    int width, height;
    CameraFrame frame;
    void *y_uv[2];
    int index = 0;
    int stride = 4096;
    char *fp = NULL;

    mParams.getPreviewSize(&width, &height);

    if (mPreviewing) {

        fp = this->GetFrame(index);
        if(!fp) {
            ret = BAD_VALUE;
            goto EXIT;
        }
        CameraBuffer *buffer = mPreviewBufs.keyAt(index);
        CameraFrame *lframe = (CameraFrame *)mFrameQueue.valueFor(buffer);
        if (!lframe) {
            ret = BAD_VALUE;
            goto EXIT;
        }

        debugShowFPS();

        if ( mFrameSubscribers.size() == 0 ) {
            ret = BAD_VALUE;
            goto EXIT;
        }
        y_uv[0] = (void*) lframe->mYuv[0];
        //y_uv[1] = (void*) lframe->mYuv[1];
        //y_uv[1] = (void*) (lframe->mYuv[0] + height*stride);
        convertYUV422ToNV12Tiler ( (unsigned char*)fp, (unsigned char*)y_uv[0], width, height);
        CAMHAL_LOGVB("##...index= %d.;camera buffer= 0x%x; y= 0x%x; UV= 0x%x.",index, buffer, y_uv[0], y_uv[1] );

#ifdef SAVE_RAW_FRAMES
        unsigned char* nv12_buff = (unsigned char*) malloc(width*height*3/2);
        //Convert yuv422i to yuv420sp(NV12) & dump the frame to a file
        convertYUV422ToNV12 ( (unsigned char*)fp, nv12_buff, width, height);
        saveFile( nv12_buff, ((width*height)*3/2) );
        free (nv12_buff);
#endif

        frame.mFrameType = CameraFrame::PREVIEW_FRAME_SYNC;
        frame.mBuffer = buffer;
        frame.mLength = width*height*3/2;
        frame.mAlignment = stride;
        frame.mOffset = 0;
        frame.mTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
        frame.mFrameMask = (unsigned int)CameraFrame::PREVIEW_FRAME_SYNC;

        if (mRecording)
        {
            frame.mFrameMask |= (unsigned int)CameraFrame::VIDEO_FRAME_SYNC;
            mFramesWithEncoder++;
        }

        ret = setInitFrameRefCount(frame.mBuffer, frame.mFrameMask);
        if (ret != NO_ERROR) {
            CAMHAL_LOGDB("Error in setInitFrameRefCount %d", ret);
        } else {
            ret = sendFrameToSubscribers(&frame);
        }
    }
EXIT:

    return ret;
}

//scan for video devices
void detectVideoDevice(char** video_device_list, int& num_device) {
    char dir_path[20];
    char* filename;
    char** dev_list = video_device_list;
    DIR *d;
    struct dirent *dir;
    int index = 0;

    strcpy(dir_path, DEVICE_PATH);
    d = opendir(dir_path);
    if(d) {
        //read each entry in the /dev/ and find if there is videox entry.
        while ((dir = readdir(d)) != NULL) {
            filename = dir->d_name;
            if (strncmp(filename, DEVICE_NAME, 5) == 0) {
                strcpy(dev_list[index],DEVICE_PATH);
                strncat(dev_list[index],filename,sizeof(DEVICE_NAME));
                index++;
            }
       } //end of while()
       closedir(d);
       num_device = index;

       for(int i=0; i<index; i++){
           CAMHAL_LOGDB("Video device list::dev_list[%d]= %s",i,dev_list[i]);
       }
    }
}

extern "C" CameraAdapter* V4LCameraAdapter_Factory(size_t sensor_index)
{
    CameraAdapter *adapter = NULL;
    android::AutoMutex lock(gV4LAdapterLock);

    LOG_FUNCTION_NAME;

    adapter = new V4LCameraAdapter(sensor_index);
    if ( adapter ) {
        CAMHAL_LOGDB("New V4L Camera adapter instance created for sensor %d",sensor_index);
    } else {
        CAMHAL_LOGEA("V4L Camera adapter create failed for sensor index = %d!",sensor_index);
    }

    LOG_FUNCTION_NAME_EXIT;

    return adapter;
}

extern "C" status_t V4LCameraAdapter_Capabilities(
        CameraProperties::Properties * const properties_array,
        const int starting_camera, const int max_camera, int & supportedCameras)
{
    status_t ret = NO_ERROR;
    struct v4l2_capability cap;
    int tempHandle = NULL;
    int num_cameras_supported = 0;
    char device_list[5][15];
    char* video_device_list[5];
    int num_v4l_devices = 0;
    int sensorId = 0;
    CameraProperties::Properties* properties = NULL;

    LOG_FUNCTION_NAME;

    supportedCameras = 0;
    memset((void*)&cap, 0, sizeof(v4l2_capability));

    if (!properties_array) {
        CAMHAL_LOGEB("invalid param: properties = 0x%p", properties_array);
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    for (int i = 0; i < 5; i++) {
        video_device_list[i] = device_list[i];
    }
    //look for the connected video devices
    detectVideoDevice(video_device_list, num_v4l_devices);

    for (int i = 0; i < num_v4l_devices; i++) {
        if ( (starting_camera + num_cameras_supported) < max_camera) {
            sensorId = starting_camera + num_cameras_supported;

            CAMHAL_LOGDB("Opening device[%d] = %s..",i, video_device_list[i]);
            if ((tempHandle = open(video_device_list[i], O_RDWR)) == -1) {
                CAMHAL_LOGEB("Error while opening handle to V4L2 Camera(%s): %s",video_device_list[i], strerror(errno));
                continue;
            }

            ret = ioctl (tempHandle, VIDIOC_QUERYCAP, &cap);
            if (ret < 0) {
                CAMHAL_LOGEA("Error when querying the capabilities of the V4L Camera");
                close(tempHandle);
                continue;
            }

            //check for video capture devices
            if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
                CAMHAL_LOGEA("Error while adapter initialization: video capture not supported.");
                close(tempHandle);
                continue;
            }

            strcpy(device, video_device_list[i]);
            properties = properties_array + starting_camera + num_cameras_supported;

            //fetch capabilities for this camera
            ret = V4LCameraAdapter::getCaps( sensorId, properties, tempHandle );
            if (ret < 0) {
                CAMHAL_LOGEA("Error while getting capabilities.");
                close(tempHandle);
                continue;
            }

            num_cameras_supported++;

        }
        //For now exit this loop once a valid video capture device is found.
        //TODO: find all V4L capture devices and it capabilities
        break;
    }//end of for() loop

    supportedCameras = num_cameras_supported;
    CAMHAL_LOGDB("Number of V4L cameras detected =%d", num_cameras_supported);

EXIT:
    LOG_FUNCTION_NAME_EXIT;
    close(tempHandle);
    return NO_ERROR;
}

} // namespace Camera
} // namespace Ti


/*--------------------Camera Adapter Class ENDS here-----------------------------*/

