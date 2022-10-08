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



#ifndef ANDROID_HARDWARE_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_CAMERA_HARDWARE_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <utils/Log.h>
#include <utils/threads.h>
#include <linux/videodev2.h>
#include "binder/MemoryBase.h"
#include "binder/MemoryHeapBase.h"
#include <utils/threads.h>
#include <camera/CameraParameters.h>
#include <hardware/camera.h>
#include "MessageQueue.h"
#include "Semaphore.h"
#include "CameraProperties.h"
#include "DebugUtils.h"
#include "SensorListener.h"

#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBuffer.h>

#define MIN_WIDTH           640
#define MIN_HEIGHT          480
#define PICTURE_WIDTH   3264 /* 5mp - 2560. 8mp - 3280 */ /* Make sure it is a multiple of 16. */
#define PICTURE_HEIGHT  2448 /* 5mp - 2048. 8mp - 2464 */ /* Make sure it is a multiple of 16. */
#define PREVIEW_WIDTH 176
#define PREVIEW_HEIGHT 144
#define PIXEL_FORMAT           V4L2_PIX_FMT_UYVY

#define VIDEO_FRAME_COUNT_MAX    8 //NUM_OVERLAY_BUFFERS_REQUESTED
#define MAX_CAMERA_BUFFERS    8 //NUM_OVERLAY_BUFFERS_REQUESTED
#define MAX_ZOOM        3
#define THUMB_WIDTH     80
#define THUMB_HEIGHT    60
#define PIX_YUV422I 0
#define PIX_YUV420P 1

#define SATURATION_OFFSET 100
#define SHARPNESS_OFFSET 100
#define CONTRAST_OFFSET 100

#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_HW_TEXTURE | \
                             GRALLOC_USAGE_HW_RENDER | \
                             GRALLOC_USAGE_SW_READ_RARELY | \
                             GRALLOC_USAGE_SW_WRITE_NEVER

//Enables Absolute PPM measurements in logcat
#define PPM_INSTRUMENTATION_ABS 1

#define LOCK_BUFFER_TRIES 5
#define HAL_PIXEL_FORMAT_NV12 0x100

#define CAMHAL_LOGI ALOGI

//Uncomment to enable more verbose/debug logs
//#define DEBUG_LOG

///Camera HAL Logging Functions
#ifndef DEBUG_LOG

#define CAMHAL_LOGDA(str)
#define CAMHAL_LOGDB(str, ...)
#define CAMHAL_LOGVA(str)
#define CAMHAL_LOGVB(str, ...)

#define CAMHAL_LOGEA ALOGE
#define CAMHAL_LOGEB ALOGE

#undef LOG_FUNCTION_NAME
#undef LOG_FUNCTION_NAME_EXIT
#define LOG_FUNCTION_NAME
#define LOG_FUNCTION_NAME_EXIT

#else

#define CAMHAL_LOGDA DBGUTILS_LOGDA
#define CAMHAL_LOGDB DBGUTILS_LOGDB
#define CAMHAL_LOGVA DBGUTILS_LOGVA
#define CAMHAL_LOGVB DBGUTILS_LOGVB

#define CAMHAL_LOGEA DBGUTILS_LOGEA
#define CAMHAL_LOGEB DBGUTILS_LOGEB

#endif



#define NONNEG_ASSIGN(x,y) \
    if(x > -1) \
        y = x

namespace android {

#define PARAM_BUFFER            6000

///Forward declarations
class CameraHal;
class CameraFrame;
class CameraHalEvent;
class DisplayFrame;

class CameraArea : public RefBase
{
public:

    CameraArea(ssize_t top,
               ssize_t left,
               ssize_t bottom,
               ssize_t right,
               size_t weight) : mTop(top),
                                mLeft(left),
                                mBottom(bottom),
                                mRight(right),
                                mWeight(weight) {}

    status_t transfrom(size_t width,
                       size_t height,
                       size_t &top,
                       size_t &left,
                       size_t &areaWidth,
                       size_t &areaHeight);

    bool isValid()
        {
        return ( ( 0 != mTop ) || ( 0 != mLeft ) || ( 0 != mBottom ) || ( 0 != mRight) );
        }

    bool isZeroArea()
    {
        return  ( (0 == mTop ) && ( 0 == mLeft ) && ( 0 == mBottom )
                 && ( 0 == mRight ) && ( 0 == mWeight ));
    }

    size_t getWeight()
        {
        return mWeight;
        }

    bool compare(const sp<CameraArea> &area);

    static status_t parseAreas(const char *area,
                               size_t areaLength,
                               Vector< sp<CameraArea> > &areas);

    static status_t checkArea(ssize_t top,
                              ssize_t left,
                              ssize_t bottom,
                              ssize_t right,
                              ssize_t weight);

    static bool areAreasDifferent(Vector< sp<CameraArea> > &, Vector< sp<CameraArea> > &);

protected:
    static const ssize_t TOP = -1000;
    static const ssize_t LEFT = -1000;
    static const ssize_t BOTTOM = 1000;
    static const ssize_t RIGHT = 1000;
    static const ssize_t WEIGHT_MIN = 1;
    static const ssize_t WEIGHT_MAX = 1000;

    ssize_t mTop;
    ssize_t mLeft;
    ssize_t mBottom;
    ssize_t mRight;
    size_t mWeight;
};

class CameraFDResult : public RefBase
{
public:

    CameraFDResult() : mFaceData(NULL) {};
    CameraFDResult(camera_frame_metadata_t *faces) : mFaceData(faces) {};

    virtual ~CameraFDResult() {
        if ( ( NULL != mFaceData ) && ( NULL != mFaceData->faces ) ) {
            free(mFaceData->faces);
            free(mFaceData);
            mFaceData=NULL;
        }

        if(( NULL != mFaceData ))
            {
            free(mFaceData);
            mFaceData = NULL;
            }
    }

    camera_frame_metadata_t *getFaceResult() { return mFaceData; };

    static const ssize_t TOP = -1000;
    static const ssize_t LEFT = -1000;
    static const ssize_t BOTTOM = 1000;
    static const ssize_t RIGHT = 1000;
    static const ssize_t INVALID_DATA = -2000;

private:

    camera_frame_metadata_t *mFaceData;
};

class CameraFrame
{
    public:

    enum FrameType
        {
            PREVIEW_FRAME_SYNC = 0x1, ///SYNC implies that the frame needs to be explicitly returned after consuming in order to be filled by camera again
            PREVIEW_FRAME = 0x2   , ///Preview frame includes viewfinder and snapshot frames
            IMAGE_FRAME_SYNC = 0x4, ///Image Frame is the image capture output frame
            IMAGE_FRAME = 0x8,
            VIDEO_FRAME_SYNC = 0x10, ///Timestamp will be updated for these frames
            VIDEO_FRAME = 0x20,
            FRAME_DATA_SYNC = 0x40, ///Any extra data assosicated with the frame. Always synced with the frame
            FRAME_DATA= 0x80,
            RAW_FRAME = 0x100,
            SNAPSHOT_FRAME = 0x200,
            ALL_FRAMES = 0xFFFF   ///Maximum of 16 frame types supported
        };

    enum FrameQuirks
    {
        ENCODE_RAW_YUV422I_TO_JPEG = 0x1 << 0,
        HAS_EXIF_DATA = 0x1 << 1,
    };

    //default contrustor
    CameraFrame():
    mCookie(NULL),
    mCookie2(NULL),
    mBuffer(NULL),
    mFrameType(0),
    mTimestamp(0),
    mWidth(0),
    mHeight(0),
    mOffset(0),
    mAlignment(0),
    mFd(0),
    mLength(0),
    mFrameMask(0),
    mQuirks(0) {

      mYuv[0] = NULL;
      mYuv[1] = NULL;
    }

    //copy constructor
    CameraFrame(const CameraFrame &frame) :
    mCookie(frame.mCookie),
    mCookie2(frame.mCookie2),
    mBuffer(frame.mBuffer),
    mFrameType(frame.mFrameType),
    mTimestamp(frame.mTimestamp),
    mWidth(frame.mWidth),
    mHeight(frame.mHeight),
    mOffset(frame.mOffset),
    mAlignment(frame.mAlignment),
    mFd(frame.mFd),
    mLength(frame.mLength),
    mFrameMask(frame.mFrameMask),
    mQuirks(frame.mQuirks) {

      mYuv[0] = frame.mYuv[0];
      mYuv[1] = frame.mYuv[1];
    }

    void *mCookie;
    void *mCookie2;
    void *mBuffer;
    int mFrameType;
    nsecs_t mTimestamp;
    unsigned int mWidth, mHeight;
    uint32_t mOffset;
    unsigned int mAlignment;
    int mFd;
    size_t mLength;
    unsigned mFrameMask;
    unsigned int mQuirks;
    unsigned int mYuv[2];
    ///@todo add other member vars like  stride etc
};

enum CameraHalError
{
    CAMERA_ERROR_FATAL = 0x1, //Fatal errors can only be recovered by restarting media server
    CAMERA_ERROR_HARD = 0x2,  // Hard errors are hardware hangs that may be recoverable by resetting the hardware internally within the adapter
    CAMERA_ERROR_SOFT = 0x4, // Soft errors are non fatal errors that can be recovered from without needing to stop use-case
};

///Common Camera Hal Event class which is visible to CameraAdapter,DisplayAdapter and AppCallbackNotifier
///@todo Rename this class to CameraEvent
class CameraHalEvent
{
public:
    //Enums
    enum CameraHalEventType {
        NO_EVENTS = 0x0,
        EVENT_FOCUS_LOCKED = 0x1,
        EVENT_FOCUS_ERROR = 0x2,
        EVENT_ZOOM_INDEX_REACHED = 0x4,
        EVENT_SHUTTER = 0x8,
        EVENT_FACE = 0x10,
        ///@remarks Future enum related to display, like frame displayed event, could be added here
        ALL_EVENTS = 0xFFFF ///Maximum of 16 event types supported
    };

    enum FocusStatus {
        FOCUS_STATUS_SUCCESS = 0x1,
        FOCUS_STATUS_FAIL = 0x2,
        FOCUS_STATUS_PENDING = 0x4,
        FOCUS_STATUS_DONE = 0x8,
    };

    ///Class declarations
    ///@remarks Add a new class for a new event type added above

    //Shutter event specific data
    typedef struct ShutterEventData_t {
        bool shutterClosed;
    }ShutterEventData;

    ///Focus event specific data
    typedef struct FocusEventData_t {
        FocusStatus focusStatus;
        int currentFocusValue;
    } FocusEventData;

    ///Zoom specific event data
    typedef struct ZoomEventData_t {
        int currentZoomIndex;
        bool targetZoomIndexReached;
    } ZoomEventData;

    typedef struct FaceData_t {
        ssize_t top;
        ssize_t left;
        ssize_t bottom;
        ssize_t right;
        size_t score;
    } FaceData;

    typedef sp<CameraFDResult> FaceEventData;

    class CameraHalEventData : public RefBase{

    public:

        CameraHalEvent::FocusEventData focusEvent;
        CameraHalEvent::ZoomEventData zoomEvent;
        CameraHalEvent::ShutterEventData shutterEvent;
        CameraHalEvent::FaceEventData faceEvent;
    };

    //default contrustor
    CameraHalEvent():
    mCookie(NULL),
    mEventType(NO_EVENTS) {}

    //copy constructor
    CameraHalEvent(const CameraHalEvent &event) :
        mCookie(event.mCookie),
        mEventType(event.mEventType),
        mEventData(event.mEventData) {};

    void* mCookie;
    CameraHalEventType mEventType;
    sp<CameraHalEventData> mEventData;

};

///      Have a generic callback class based on template - to adapt CameraFrame and Event
typedef void (*frame_callback) (CameraFrame *cameraFrame);
typedef void (*event_callback) (CameraHalEvent *event);

//signals CameraHAL to relase image buffers
typedef void (*release_image_buffers_callback) (void *userData);
typedef void (*end_image_capture_callback) (void *userData);

/**
  * Interface class implemented by classes that have some events to communicate to dependendent classes
  * Dependent classes use this interface for registering for events
  */
class MessageNotifier
{
public:
    static const uint32_t EVENT_BIT_FIELD_POSITION;
    static const uint32_t FRAME_BIT_FIELD_POSITION;

    ///@remarks Msg type comes from CameraFrame and CameraHalEvent classes
    ///           MSB 16 bits is for events and LSB 16 bits is for frame notifications
    ///         FrameProvider and EventProvider classes act as helpers to event/frame
    ///         consumers to call this api
    virtual void enableMsgType(int32_t msgs, frame_callback frameCb=NULL, event_callback eventCb=NULL, void* cookie=NULL) = 0;
    virtual void disableMsgType(int32_t msgs, void* cookie) = 0;

    virtual ~MessageNotifier() {};
};

class ErrorNotifier : public virtual RefBase
{
public:
    virtual void errorNotify(int error) = 0;

    virtual ~ErrorNotifier() {};
};


/**
  * Interace class abstraction for Camera Adapter to act as a frame provider
  * This interface is fully implemented by Camera Adapter
  */
class FrameNotifier : public MessageNotifier
{
public:
    virtual void returnFrame(void* frameBuf, CameraFrame::FrameType frameType) = 0;
    virtual void addFramePointers(void *frameBuf, void *buf) = 0;
    virtual void removeFramePointers() = 0;

    virtual ~FrameNotifier() {};
};

/**   * Wrapper class around Frame Notifier, which is used by display and notification classes for interacting with Camera Adapter
  */
class FrameProvider
{
    FrameNotifier* mFrameNotifier;
    void* mCookie;
    frame_callback mFrameCallback;

public:
    FrameProvider(FrameNotifier *fn, void* cookie, frame_callback frameCallback)
        :mFrameNotifier(fn), mCookie(cookie),mFrameCallback(frameCallback) { }

    int enableFrameNotification(int32_t frameTypes);
    int disableFrameNotification(int32_t frameTypes);
    int returnFrame(void *frameBuf, CameraFrame::FrameType frameType);
    void addFramePointers(void *frameBuf, void *buf);
    void removeFramePointers();
};

/** Wrapper class around MessageNotifier, which is used by display and notification classes for interacting with
   *  Camera Adapter
  */
class EventProvider
{
public:
    MessageNotifier* mEventNotifier;
    void* mCookie;
    event_callback mEventCallback;

public:
    EventProvider(MessageNotifier *mn, void* cookie, event_callback eventCallback)
        :mEventNotifier(mn), mCookie(cookie), mEventCallback(eventCallback) {}

    int enableEventNotification(int32_t eventTypes);
    int disableEventNotification(int32_t eventTypes);
};

/*
  * Interface for providing buffers
  */
class BufferProvider
{
public:
    virtual void* allocateBuffer(int width, int height, const char* format, int &bytes, int numBufs) = 0;

    //additional methods used for memory mapping
    virtual uint32_t * getOffsets() = 0;
    virtual int getFd() = 0;

    virtual int freeBuffer(void* buf) = 0;

    virtual ~BufferProvider() {}
};

/**
  * Class for handling data and notify callbacks to application
  */
class   AppCallbackNotifier: public ErrorNotifier , public virtual RefBase
{

public:

    ///Constants
    static const int NOTIFIER_TIMEOUT;
    static const int32_t MAX_BUFFERS = 8;

    enum NotifierCommands
        {
        NOTIFIER_CMD_PROCESS_EVENT,
        NOTIFIER_CMD_PROCESS_FRAME,
        NOTIFIER_CMD_PROCESS_ERROR
        };

    enum NotifierState
        {
        NOTIFIER_STOPPED,
        NOTIFIER_STARTED,
        NOTIFIER_EXITED
        };

public:

    ~AppCallbackNotifier();

    ///Initialzes the callback notifier, creates any resources required
    status_t initialize();

    ///Starts the callbacks to application
    status_t start();

    ///Stops the callbacks from going to application
    status_t stop();

    void setEventProvider(int32_t eventMask, MessageNotifier * eventProvider);
    void setFrameProvider(FrameNotifier *frameProvider);

    //All sub-components of Camera HAL call this whenever any error happens
    virtual void errorNotify(int error);

    status_t startPreviewCallbacks(CameraParameters &params, void *buffers, uint32_t *offsets, int fd, size_t length, size_t count);
    status_t stopPreviewCallbacks();

    status_t enableMsgType(int32_t msgType);
    status_t disableMsgType(int32_t msgType);

    //API for enabling/disabling measurement data
    void setMeasurements(bool enable);

    //thread loops
    bool notificationThread();

    ///Notification callback functions
    static void frameCallbackRelay(CameraFrame* caFrame);
    static void eventCallbackRelay(CameraHalEvent* chEvt);
    void frameCallback(CameraFrame* caFrame);
    void eventCallback(CameraHalEvent* chEvt);
    void flushAndReturnFrames();

    void setCallbacks(CameraHal *cameraHal,
                        camera_notify_callback notify_cb,
                        camera_data_callback data_cb,
                        camera_data_timestamp_callback data_cb_timestamp,
                        camera_request_memory get_memory,
                        void *user);

    //Set Burst mode
    void setBurst(bool burst);

    //Notifications from CameraHal for video recording case
    status_t startRecording();
    status_t stopRecording();
    status_t initSharedVideoBuffers(void *buffers, uint32_t *offsets, int fd, size_t length, size_t count, void *vidBufs);
    status_t releaseRecordingFrame(const void *opaque);

	status_t useMetaDataBufferMode(bool enable);

    void EncoderDoneCb(void*, void*, CameraFrame::FrameType type, void* cookie1, void* cookie2);

    void useVideoBuffers(bool useVideoBuffers);

    bool getUesVideoBuffers();
    void setVideoRes(int width, int height);

    void flushEventQueue();

    //Internal class definitions
    class NotificationThread : public Thread {
        AppCallbackNotifier* mAppCallbackNotifier;
        TIUTILS::MessageQueue mNotificationThreadQ;
    public:
        enum NotificationThreadCommands
        {
        NOTIFIER_START,
        NOTIFIER_STOP,
        NOTIFIER_EXIT,
        };
    public:
        NotificationThread(AppCallbackNotifier* nh)
            : Thread(false), mAppCallbackNotifier(nh) { }
        virtual bool threadLoop() {
            return mAppCallbackNotifier->notificationThread();
        }

        TIUTILS::MessageQueue &msgQ() { return mNotificationThreadQ;}
    };

    //Friend declarations
    friend class NotificationThread;

private:
    void notifyEvent();
    void notifyFrame();
    bool processMessage();
    void releaseSharedVideoBuffers();
    status_t dummyRaw();
    void copyAndSendPictureFrame(CameraFrame* frame, int32_t msgType);
    void copyAndSendPreviewFrame(CameraFrame* frame, int32_t msgType);

private:
    mutable Mutex mLock;
    mutable Mutex mBurstLock;
    CameraHal* mCameraHal;
    camera_notify_callback mNotifyCb;
    camera_data_callback   mDataCb;
    camera_data_timestamp_callback mDataCbTimestamp;
    camera_request_memory mRequestMemory;
    void *mCallbackCookie;

    //Keeps Video MemoryHeaps and Buffers within
    //these objects
    KeyedVector<unsigned int, unsigned int> mVideoHeaps;
    KeyedVector<unsigned int, unsigned int> mVideoBuffers;
    KeyedVector<unsigned int, unsigned int> mVideoMap;

    //Keeps list of Gralloc handles and associated Video Metadata Buffers
    KeyedVector<uint32_t, uint32_t> mVideoMetadataBufferMemoryMap;
    KeyedVector<uint32_t, uint32_t> mVideoMetadataBufferReverseMap;

    bool mBufferReleased;

    sp< NotificationThread> mNotificationThread;
    EventProvider *mEventProvider;
    FrameProvider *mFrameProvider;
    TIUTILS::MessageQueue mEventQ;
    TIUTILS::MessageQueue mFrameQ;
    NotifierState mNotifierState;

    bool mPreviewing;
    camera_memory_t* mPreviewMemory;
    unsigned char* mPreviewBufs[MAX_BUFFERS];
    int mPreviewBufCount;
    const char *mPreviewPixelFormat;
    KeyedVector<unsigned int, sp<MemoryHeapBase> > mSharedPreviewHeaps;
    KeyedVector<unsigned int, sp<MemoryBase> > mSharedPreviewBuffers;

    //Burst mode active
    bool mBurst;
    mutable Mutex mRecordingLock;
    bool mRecording;
    bool mMeasurementEnabled;

    bool mUseMetaDataBufferMode;
    bool mRawAvailable;

    bool mUseVideoBuffers;

    int mVideoWidth;
    int mVideoHeight;

};


/**
  * Class used for allocating memory for JPEG bit stream buffers, output buffers of camera in no overlay case
  */
class MemoryManager : public BufferProvider, public virtual RefBase
{
public:
    MemoryManager():mIonFd(-1){ }

    ///Initializes the memory manager creates any resources required
    status_t initialize() { return NO_ERROR; }

    int setErrorHandler(ErrorNotifier *errorNotifier);
    virtual void* allocateBuffer(int width, int height, const char* format, int &bytes, int numBufs);
    virtual uint32_t * getOffsets();
    virtual int getFd() ;
    virtual int freeBuffer(void* buf);

private:

    sp<ErrorNotifier> mErrorNotifier;
    int mIonFd;
    KeyedVector<unsigned int, unsigned int> mIonHandleMap;
    KeyedVector<unsigned int, unsigned int> mIonFdMap;
    KeyedVector<unsigned int, unsigned int> mIonBufLength;
};




/**
  * CameraAdapter interface class
  * Concrete classes derive from this class and provide implementations based on the specific camera h/w interface
  */

class CameraAdapter: public FrameNotifier, public virtual RefBase
{
protected:
    enum AdapterActiveStates {
        INTIALIZED_ACTIVE =     1 << 0,
        LOADED_PREVIEW_ACTIVE = 1 << 1,
        PREVIEW_ACTIVE =        1 << 2,
        LOADED_CAPTURE_ACTIVE = 1 << 3,
        CAPTURE_ACTIVE =        1 << 4,
        BRACKETING_ACTIVE =     1 << 5,
        AF_ACTIVE =             1 << 6,
        ZOOM_ACTIVE =           1 << 7,
        VIDEO_ACTIVE =          1 << 8,
    };
public:
    typedef struct
        {
         void *mBuffers;
         uint32_t *mOffsets;
         int mFd;
         size_t mLength;
         size_t mCount;
         size_t mMaxQueueable;
        } BuffersDescriptor;

    enum CameraCommands
        {
        CAMERA_START_PREVIEW                        = 0,
        CAMERA_STOP_PREVIEW                         = 1,
        CAMERA_START_VIDEO                          = 2,
        CAMERA_STOP_VIDEO                           = 3,
        CAMERA_START_IMAGE_CAPTURE                  = 4,
        CAMERA_STOP_IMAGE_CAPTURE                   = 5,
        CAMERA_PERFORM_AUTOFOCUS                    = 6,
        CAMERA_CANCEL_AUTOFOCUS                     = 7,
        CAMERA_PREVIEW_FLUSH_BUFFERS                = 8,
        CAMERA_START_SMOOTH_ZOOM                    = 9,
        CAMERA_STOP_SMOOTH_ZOOM                     = 10,
        CAMERA_USE_BUFFERS_PREVIEW                  = 11,
        CAMERA_SET_TIMEOUT                          = 12,
        CAMERA_CANCEL_TIMEOUT                       = 13,
        CAMERA_START_BRACKET_CAPTURE                = 14,
        CAMERA_STOP_BRACKET_CAPTURE                 = 15,
        CAMERA_QUERY_RESOLUTION_PREVIEW             = 16,
        CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE      = 17,
        CAMERA_QUERY_BUFFER_SIZE_PREVIEW_DATA       = 18,
        CAMERA_USE_BUFFERS_IMAGE_CAPTURE            = 19,
        CAMERA_USE_BUFFERS_PREVIEW_DATA             = 20,
        CAMERA_TIMEOUT_EXPIRED                      = 21,
        CAMERA_START_FD                             = 22,
        CAMERA_STOP_FD                              = 23,
        CAMERA_SWITCH_TO_EXECUTING                  = 24,
        };

    enum CameraMode
        {
        CAMERA_PREVIEW,
        CAMERA_IMAGE_CAPTURE,
        CAMERA_VIDEO,
        CAMERA_MEASUREMENT
        };

    enum AdapterState {
        INTIALIZED_STATE           = INTIALIZED_ACTIVE,
        LOADED_PREVIEW_STATE       = LOADED_PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        PREVIEW_STATE              = PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        LOADED_CAPTURE_STATE       = LOADED_CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        CAPTURE_STATE              = CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        BRACKETING_STATE           = BRACKETING_ACTIVE | CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE ,
        AF_STATE                   = AF_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        ZOOM_STATE                 = ZOOM_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_STATE                = VIDEO_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_AF_STATE             = VIDEO_ACTIVE | AF_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_ZOOM_STATE           = VIDEO_ACTIVE | ZOOM_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_LOADED_CAPTURE_STATE = VIDEO_ACTIVE | LOADED_CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_CAPTURE_STATE        = VIDEO_ACTIVE | CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        AF_ZOOM_STATE              = AF_ACTIVE | ZOOM_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        BRACKETING_ZOOM_STATE      = BRACKETING_ACTIVE | ZOOM_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
    };

public:

    ///Initialzes the camera adapter creates any resources required
    virtual int initialize(CameraProperties::Properties*) = 0;

    virtual int setErrorHandler(ErrorNotifier *errorNotifier) = 0;

    //Message/Frame notification APIs
    virtual void enableMsgType(int32_t msgs,
                               frame_callback callback = NULL,
                               event_callback eventCb = NULL,
                               void *cookie = NULL) = 0;
    virtual void disableMsgType(int32_t msgs, void* cookie) = 0;
    virtual void returnFrame(void* frameBuf, CameraFrame::FrameType frameType) = 0;
    virtual void addFramePointers(void *frameBuf, void *buf) = 0;
    virtual void removeFramePointers() = 0;

    //APIs to configure Camera adapter and get the current parameter set
    virtual int setParameters(const CameraParameters& params) = 0;
    virtual void getParameters(CameraParameters& params) = 0;

    //API to flush the buffers from Camera
     status_t flushBuffers()
        {
        return sendCommand(CameraAdapter::CAMERA_PREVIEW_FLUSH_BUFFERS);
        }

    //Registers callback for returning image buffers back to CameraHAL
    virtual int registerImageReleaseCallback(release_image_buffers_callback callback, void *user_data) = 0;

    //Registers callback, which signals a completed image capture
    virtual int registerEndCaptureCallback(end_image_capture_callback callback, void *user_data) = 0;

    //API to send a command to the camera
    virtual status_t sendCommand(CameraCommands operation, int value1=0, int value2=0, int value3=0) = 0;

    virtual ~CameraAdapter() {};

    //Retrieves the current Adapter state
    virtual AdapterState getState() = 0;

    //Retrieves the next Adapter state
    virtual AdapterState getNextState() = 0;

    // Receive orientation events from CameraHal
    virtual void onOrientationEvent(uint32_t orientation, uint32_t tilt) = 0;

    // Rolls the state machine back to INTIALIZED_STATE from the current state
    virtual status_t rollbackToInitializedState() = 0;

    // Retrieves the current Adapter state - for internal use (not locked)
    virtual status_t getState(AdapterState &state) = 0;
    // Retrieves the next Adapter state - for internal use (not locked)
    virtual status_t getNextState(AdapterState &state) = 0;

protected:
    //The first two methods will try to switch the adapter state.
    //Every call to setState() should be followed by a corresponding
    //call to commitState(). If the state switch fails, then it will
    //get reset to the previous state via rollbackState().
    virtual status_t setState(CameraCommands operation) = 0;
    virtual status_t commitState() = 0;
    virtual status_t rollbackState() = 0;
};

class DisplayAdapter : public BufferProvider, public virtual RefBase
{
public:
    typedef struct S3DParameters_t
    {
        int mode;
        int framePacking;
        int order;
        int subSampling;
    } S3DParameters;

    ///Initializes the display adapter creates any resources required
    virtual int initialize() = 0;

    virtual int setPreviewWindow(struct preview_stream_ops *window) = 0;
    virtual int setFrameProvider(FrameNotifier *frameProvider) = 0;
    virtual int setErrorHandler(ErrorNotifier *errorNotifier) = 0;
    virtual int enableDisplay(int width, int height, struct timeval *refTime = NULL, S3DParameters *s3dParams = NULL) = 0;
    virtual int disableDisplay(bool cancel_buffer = true) = 0;
    //Used for Snapshot review temp. pause
    virtual int pauseDisplay(bool pause) = 0;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
    //Used for shot to snapshot measurement
    virtual int setSnapshotTimeRef(struct timeval *refTime = NULL) = 0;
#endif

    virtual int useBuffers(void *bufArr, int num) = 0;
    virtual bool supportsExternalBuffering() = 0;

    // Get max queueable buffers display supports
    // This function should only be called after
    // allocateBuffer
    virtual int maxQueueableBuffers(unsigned int& queueable) = 0;
};

static void releaseImageBuffers(void *userData);

static void endImageCapture(void *userData);

 /**
    Implementation of the Android Camera hardware abstraction layer

    This class implements the interface methods defined in CameraHardwareInterface
    for the OMAP4 platform

*/
class CameraHal

{

public:
    ///Constants
    static const int NO_BUFFERS_PREVIEW;
    static const int NO_BUFFERS_IMAGE_CAPTURE;
    static const uint32_t VFR_SCALE = 1000;


    /*--------------------Interface Methods---------------------------------*/

     //@{
public:

    /** Set the notification and data callbacks */
    void setCallbacks(camera_notify_callback notify_cb,
                        camera_data_callback data_cb,
                        camera_data_timestamp_callback data_cb_timestamp,
                        camera_request_memory get_memory,
                        void *user);

    /** Receives orientation events from SensorListener **/
    void onOrientationEvent(uint32_t orientation, uint32_t tilt);

    /**
     * The following three functions all take a msgtype,
     * which is a bitmask of the messages defined in
     * include/ui/Camera.h
     */

    /**
     * Enable a message, or set of messages.
     */
    void        enableMsgType(int32_t msgType);

    /**
     * Disable a message, or a set of messages.
     */
    void        disableMsgType(int32_t msgType);

    /**
     * Query whether a message, or a set of messages, is enabled.
     * Note that this is operates as an AND, if any of the messages
     * queried are off, this will return false.
     */
    int        msgTypeEnabled(int32_t msgType);

    /**
     * Start preview mode.
     */
    int    startPreview();

    /**
     * Only used if overlays are used for camera preview.
     */
    int setPreviewWindow(struct preview_stream_ops *window);

    /**
     * Stop a previously started preview.
     */
    void        stopPreview();

    /**
     * Returns true if preview is enabled.
     */
    bool        previewEnabled();

    /**
     * Start record mode. When a record image is available a CAMERA_MSG_VIDEO_FRAME
     * message is sent with the corresponding frame. Every record frame must be released
     * by calling releaseRecordingFrame().
     */
    int    startRecording();

    /**
     * Stop a previously started recording.
     */
    void        stopRecording();

    /**
     * Returns true if recording is enabled.
     */
    int        recordingEnabled();

    /**
     * Release a record frame previously returned by CAMERA_MSG_VIDEO_FRAME.
     */
    void        releaseRecordingFrame(const void *opaque);

    /**
     * Start auto focus, the notification callback routine is called
     * with CAMERA_MSG_FOCUS once when focusing is complete. autoFocus()
     * will be called again if another auto focus is needed.
     */
    int    autoFocus();

    /**
     * Cancels auto-focus function. If the auto-focus is still in progress,
     * this function will cancel it. Whether the auto-focus is in progress
     * or not, this function will return the focus position to the default.
     * If the camera does not support auto-focus, this is a no-op.
     */
    int    cancelAutoFocus();

    /**
     * Take a picture.
     */
    int    takePicture();

    /**
     * Cancel a picture that was started with takePicture.  Calling this
     * method when no picture is being taken is a no-op.
     */
    int    cancelPicture();

    /** Set the camera parameters. */
    int    setParameters(const char* params);
    int    setParameters(const CameraParameters& params);

    /** Return the camera parameters. */
    char*  getParameters();
    void putParameters(char *);

    /**
     * Send command to camera driver.
     */
    int sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);

    /**
     * Release the hardware resources owned by this object.  Note that this is
     * *not* done in the destructor.
     */
    void release();

    /**
     * Dump state of the camera hardware
     */
    int dump(int fd) const;


		status_t storeMetaDataInBuffers(bool enable);

     //@}

/*--------------------Internal Member functions - Public---------------------------------*/

public:
 /** @name internalFunctionsPublic */
  //@{

    /** Constructor of CameraHal */
    CameraHal(int cameraId);

    // Destructor of CameraHal
    ~CameraHal();

    /** Initialize CameraHal */
    status_t initialize(CameraProperties::Properties*);

    /** Deinitialize CameraHal */
    void deinitialize();

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //Uses the constructor timestamp as a reference to calcluate the
    // elapsed time
    static void PPM(const char *);
    //Uses a user provided timestamp as a reference to calcluate the
    // elapsed time
    static void PPM(const char *, struct timeval*, ...);

#endif

    /** Free image bufs */
    status_t freeImageBufs();

    //Signals the end of image capture
    status_t signalEndImageCapture();

    //Events
    static void eventCallbackRelay(CameraHalEvent* event);
    void eventCallback(CameraHalEvent* event);
    void setEventProvider(int32_t eventMask, MessageNotifier * eventProvider);

/*--------------------Internal Member functions - Private---------------------------------*/
private:

    /** @name internalFunctionsPrivate */
    //@{

    /**  Set the camera parameters specific to Video Recording. */
    bool        setVideoModeParameters(const CameraParameters&);

    /** Reset the camera parameters specific to Video Recording. */
    bool       resetVideoModeParameters();

    /** Restart the preview with setParameter. */
    status_t        restartPreview();

    status_t parseResolution(const char *resStr, int &width, int &height);

    void insertSupportedParams();

    /** Allocate preview data buffers */
    status_t allocPreviewDataBufs(size_t size, size_t bufferCount);

    /** Free preview data buffers */
    status_t freePreviewDataBufs();

    /** Allocate preview buffers */
    status_t allocPreviewBufs(int width, int height, const char* previewFormat, unsigned int bufferCount, unsigned int &max_queueable);

    /** Allocate video buffers */
    status_t allocVideoBufs(uint32_t width, uint32_t height, uint32_t bufferCount);

    /** Allocate image capture buffers */
    status_t allocImageBufs(unsigned int width, unsigned int height, size_t length, const char* previewFormat, unsigned int bufferCount);

    /** Free preview buffers */
    status_t freePreviewBufs();

    /** Free video bufs */
    status_t freeVideoBufs(void *bufs);

    //Check if a given resolution is supported by the current camera
    //instance
    bool isResolutionValid(unsigned int width, unsigned int height, const char *supportedResolutions);

    //Check if a given parameter is supported by the current camera
    // instance
    bool isParameterValid(const char *param, const char *supportedParams);
    bool isParameterValid(int param, const char *supportedParams);
    status_t doesSetParameterNeedUpdate(const char *new_param, const char *old_params, bool &update);

    /** Initialize default parameters */
    void initDefaultParameters();

    void dumpProperties(CameraProperties::Properties& cameraProps);

    status_t startImageBracketing();

    status_t stopImageBracketing();

    void setShutter(bool enable);

    void forceStopPreview();

    void selectFPSRange(int framerate, int *min_fps, int *max_fps);

    void setPreferredPreviewRes(int width, int height);
    void resetPreviewRes(CameraParameters *mParams, int width, int height);

    //@}


/*----------Member variables - Public ---------------------*/
public:
    int32_t mMsgEnabled;
    bool mRecordEnabled;
    nsecs_t mCurrentTime;
    bool mFalsePreview;
    bool mPreviewEnabled;
    uint32_t mTakePictureQueue;
    bool mBracketingEnabled;
    bool mBracketingRunning;
    //User shutter override
    bool mShutterEnabled;
    bool mMeasurementEnabled;
    //Google's parameter delimiter
    static const char PARAMS_DELIMITER[];

    CameraAdapter *mCameraAdapter;
    sp<AppCallbackNotifier> mAppCallbackNotifier;
    sp<DisplayAdapter> mDisplayAdapter;
    sp<MemoryManager> mMemoryManager;

    sp<IMemoryHeap> mPictureHeap;

    int* mGrallocHandles;
    bool mFpsRangeChangedByApp;





///static member vars

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //Timestamp from the CameraHal constructor
    static struct timeval ppm_start;
    //Timestamp of the autoFocus command
    static struct timeval mStartFocus;
    //Timestamp of the startPreview command
    static struct timeval mStartPreview;
    //Timestamp of the takePicture command
    static struct timeval mStartCapture;

#endif

/*----------Member variables - Private ---------------------*/
private:
    bool mDynamicPreviewSwitch;
    //keeps paused state of display
    bool mDisplayPaused;
    //Index of current camera adapter
    int mCameraIndex;

    mutable Mutex mLock;

    sp<SensorListener> mSensorListener;

    void* mCameraAdapterHandle;

    CameraParameters mParameters;
    bool mPreviewRunning;
    bool mPreviewStateOld;
    bool mRecordingEnabled;
    EventProvider *mEventProvider;

    int32_t *mPreviewDataBufs;
    uint32_t *mPreviewDataOffsets;
    int mPreviewDataFd;
    int mPreviewDataLength;
    int32_t *mImageBufs;
    uint32_t *mImageOffsets;
    int mImageFd;
    int mImageLength;
    int32_t *mPreviewBufs;
    uint32_t *mPreviewOffsets;
    int mPreviewLength;
    int mPreviewFd;
    int32_t *mVideoBufs;
    uint32_t *mVideoOffsets;
    int mVideoFd;
    int mVideoLength;

    int mBracketRangePositive;
    int mBracketRangeNegative;

    ///@todo Rename this as preview buffer provider
    BufferProvider *mBufProvider;
    BufferProvider *mVideoBufProvider;


    CameraProperties::Properties* mCameraProperties;

    bool mPreviewStartInProgress;

    bool mSetPreviewWindowCalled;

    uint32_t mPreviewWidth;
    uint32_t mPreviewHeight;
    int32_t mMaxZoomSupported;

    int mVideoWidth;
    int mVideoHeight;

};


}; // namespace android

#endif
