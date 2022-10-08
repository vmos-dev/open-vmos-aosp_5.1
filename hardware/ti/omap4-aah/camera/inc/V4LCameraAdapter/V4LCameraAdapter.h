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

#include <linux/videodev2.h>

#include "CameraHal.h"
#include "BaseCameraAdapter.h"
#include "DebugUtils.h"

namespace Ti {
namespace Camera {

#define DEFAULT_PIXEL_FORMAT V4L2_PIX_FMT_YUYV

#define NB_BUFFER 10
#define DEVICE "/dev/videoxx"
#define DEVICE_PATH "/dev/"
#define DEVICE_NAME "videoxx"

typedef int V4L_HANDLETYPE;

struct CapPixelformat {
        uint32_t pixelformat;
        const char *param;
};

struct CapResolution {
        size_t width, height;
        char param[10];
};

struct CapU32 {
        uint32_t num;
        const char *param;
};

typedef CapU32 CapFramerate;

struct VideoInfo {
    struct v4l2_capability cap;
    struct v4l2_format format;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers rb;
    void *mem[NB_BUFFER];
    void *CaptureBuffers[NB_BUFFER];
    bool isStreaming;
    int width;
    int height;
    int formatIn;
    int framesizeIn;
};

typedef struct V4L_TI_CAPTYPE {
    uint16_t        ulPreviewFormatCount;   // supported preview pixelformat count
    uint32_t        ePreviewFormats[32];
    uint16_t        ulPreviewResCount;   // supported preview resolution sizes
    CapResolution   tPreviewRes[32];
    uint16_t        ulCaptureResCount;   // supported capture resolution sizes
    CapResolution   tCaptureRes[32];
    uint16_t        ulFrameRateCount;   // supported frame rate
    uint16_t        ulFrameRates[32];
}V4L_TI_CAPTYPE;

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

    V4LCameraAdapter(size_t sensor_index);
    ~V4LCameraAdapter();


    ///Initialzes the camera adapter creates any resources required
    virtual status_t initialize(CameraProperties::Properties*);

    //APIs to configure Camera adapter and get the current parameter set
    virtual status_t setParameters(const android::CameraParameters& params);
    virtual void getParameters(android::CameraParameters& params);

    // API
    virtual status_t UseBuffersPreview(CameraBuffer *bufArr, int num);
    virtual status_t UseBuffersCapture(CameraBuffer *bufArr, int num);

    static status_t getCaps(const int sensorId, CameraProperties::Properties* params, V4L_HANDLETYPE handle);

protected:

//----------Parent class method implementation------------------------------------
    virtual status_t startPreview();
    virtual status_t stopPreview();
    virtual status_t takePicture();
    virtual status_t stopImageCapture();
    virtual status_t autoFocus();
    virtual status_t useBuffers(CameraMode mode, CameraBuffer *bufArr, int num, size_t length, unsigned int queueable);
    virtual status_t fillThisBuffer(CameraBuffer *frameBuf, CameraFrame::FrameType frameType);
    virtual status_t getFrameSize(size_t &width, size_t &height);
    virtual status_t getPictureBufferSize(CameraFrame &frame, size_t bufferCount);
    virtual status_t getFrameDataSize(size_t &dataFrameSize, size_t bufferCount);
    virtual void onOrientationEvent(uint32_t orientation, uint32_t tilt);
//-----------------------------------------------------------------------------


private:

    class PreviewThread : public android::Thread {
            V4LCameraAdapter* mAdapter;
        public:
            PreviewThread(V4LCameraAdapter* hw) :
                    Thread(false), mAdapter(hw) { }
            virtual void onFirstRef() {
                run("CameraPreviewThread", android::PRIORITY_URGENT_DISPLAY);
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
    //capabilities data
    static const CapPixelformat mPixelformats [];
    static const CapResolution mPreviewRes [];
    static const CapFramerate mFramerates [];
    static const CapResolution mImageCapRes [];

    //camera defaults
    static const char DEFAULT_PREVIEW_FORMAT[];
    static const char DEFAULT_PREVIEW_SIZE[];
    static const char DEFAULT_FRAMERATE[];
    static const char DEFAULT_NUM_PREV_BUFS[];

    static const char DEFAULT_PICTURE_FORMAT[];
    static const char DEFAULT_PICTURE_SIZE[];
    static const char DEFAULT_FOCUS_MODE[];
    static const char * DEFAULT_VSTAB;
    static const char * DEFAULT_VNF;

    static status_t insertDefaults(CameraProperties::Properties*, V4L_TI_CAPTYPE&);
    static status_t insertCapabilities(CameraProperties::Properties*, V4L_TI_CAPTYPE&);
    static status_t insertPreviewFormats(CameraProperties::Properties* , V4L_TI_CAPTYPE&);
    static status_t insertPreviewSizes(CameraProperties::Properties* , V4L_TI_CAPTYPE&);
    static status_t insertImageSizes(CameraProperties::Properties* , V4L_TI_CAPTYPE&);
    static status_t insertFrameRates(CameraProperties::Properties* , V4L_TI_CAPTYPE&);
    static status_t sortAscend(V4L_TI_CAPTYPE&, uint16_t ) ;

    status_t v4lIoctl(int, int, void*);
    status_t v4lInitMmap(int&);
    status_t v4lInitUsrPtr(int&);
    status_t v4lStartStreaming();
    status_t v4lStopStreaming(int nBufferCount);
    status_t v4lSetFormat(int, int, uint32_t);
    status_t restartPreview();


    int mPreviewBufferCount;
    int mPreviewBufferCountQueueable;
    int mCaptureBufferCount;
    int mCaptureBufferCountQueueable;
    android::KeyedVector<CameraBuffer *, int> mPreviewBufs;
    android::KeyedVector<CameraBuffer *, int> mCaptureBufs;
    mutable android::Mutex mPreviewBufsLock;
    mutable android::Mutex mCaptureBufsLock;
    mutable android::Mutex mStopPreviewLock;

    android::CameraParameters mParams;

    bool mPreviewing;
    bool mCapturing;
    android::Mutex mLock;

    int mFrameCount;
    int mLastFrameCount;
    unsigned int mIter;
    nsecs_t mLastFPSTime;

    //variables holding the estimated framerate
    float mFPS, mLastFPS;

    int mSensorIndex;

    // protected by mLock
    android::sp<PreviewThread>   mPreviewThread;

    struct VideoInfo *mVideoInfo;
    int mCameraHandle;

    int nQueued;
    int nDequeued;

};

} // namespace Camera
} // namespace Ti

#endif //V4L_CAMERA_ADAPTER_H
