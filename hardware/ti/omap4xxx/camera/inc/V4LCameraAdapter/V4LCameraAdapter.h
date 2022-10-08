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



#ifndef V4L_CAMERA_ADAPTER_H
#define V4L_CAMERA_ADAPTER_H

#include "CameraHal.h"
#include "BaseCameraAdapter.h"
#include "DebugUtils.h"

namespace android {

#define DEFAULT_PIXEL_FORMAT V4L2_PIX_FMT_YUYV
#define NB_BUFFER 10
#define DEVICE "/dev/video4"


struct VideoInfo {
    struct v4l2_capability cap;
    struct v4l2_format format;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers rb;
    void *mem[NB_BUFFER];
    bool isStreaming;
    int width;
    int height;
    int formatIn;
    int framesizeIn;
};


/**
  * Class which completely abstracts the camera hardware interaction from camera hal
  * TODO: Need to list down here, all the message types that will be supported by this class
                Need to implement BufferProvider interface to use AllocateBuffer of OMX if needed
  */
class V4LCameraAdapter : public BaseCameraAdapter
{
public:

    /*--------------------Constant declarations----------------------------------------*/
    static const int32_t MAX_NO_BUFFERS = 20;

    ///@remarks OMX Camera has six ports - buffer input, time input, preview, image, video, and meta data
    static const int MAX_NO_PORTS = 6;

    ///Five second timeout
    static const int CAMERA_ADAPTER_TIMEOUT = 5000*1000;

public:

    V4LCameraAdapter();
    ~V4LCameraAdapter();


    ///Initialzes the camera adapter creates any resources required
    virtual status_t initialize(CameraProperties::Properties*, int sensor_index=0);

    //APIs to configure Camera adapter and get the current parameter set
    virtual status_t setParameters(const CameraParameters& params);
    virtual void getParameters(CameraParameters& params);

    // API
    virtual status_t UseBuffersPreview(void* bufArr, int num);

    //API to flush the buffers for preview
    status_t flushBuffers();

protected:

//----------Parent class method implementation------------------------------------
    virtual status_t startPreview();
    virtual status_t stopPreview();
    virtual status_t useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable);
    virtual status_t fillThisBuffer(void* frameBuf, CameraFrame::FrameType frameType);
    virtual status_t getFrameSize(size_t &width, size_t &height);
    virtual status_t getPictureBufferSize(size_t &length, size_t bufferCount);
    virtual status_t getFrameDataSize(size_t &dataFrameSize, size_t bufferCount);
    virtual void onOrientationEvent(uint32_t orientation, uint32_t tilt);
//-----------------------------------------------------------------------------


private:

    class PreviewThread : public Thread {
            V4LCameraAdapter* mAdapter;
        public:
            PreviewThread(V4LCameraAdapter* hw) :
                    Thread(false), mAdapter(hw) { }
            virtual void onFirstRef() {
                run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
            }
            virtual bool threadLoop() {
                mAdapter->previewThread();
                // loop until we need to quit
                return true;
            }
        };

    //Used for calculation of the average frame rate during preview
    status_t recalculateFPS();

    char * GetFrame(int &index);

    int previewThread();

public:

private:
    int mPreviewBufferCount;
    KeyedVector<int, int> mPreviewBufs;
    mutable Mutex mPreviewBufsLock;

    CameraParameters mParams;

    bool mPreviewing;
    bool mCapturing;
    Mutex mLock;

    int mFrameCount;
    int mLastFrameCount;
    unsigned int mIter;
    nsecs_t mLastFPSTime;

    //variables holding the estimated framerate
    float mFPS, mLastFPS;

    int mSensorIndex;

     // protected by mLock
     sp<PreviewThread>   mPreviewThread;

     struct VideoInfo *mVideoInfo;
     int mCameraHandle;


    int nQueued;
    int nDequeued;

};
}; //// namespace
#endif //V4L_CAMERA_ADAPTER_H

