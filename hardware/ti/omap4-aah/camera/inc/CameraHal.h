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

#include <hardware/camera.h>
#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/threads.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <camera/CameraParameters.h>
#ifdef OMAP_ENHANCEMENT_CPCAM
#include <camera/CameraMetadata.h>
#include <camera/ShotParameters.h>
#endif
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBuffer.h>

/* For IMG_native_handle_t */
#include <ui/GraphicBufferMapper.h>
#include <hal_public.h>

#include <ion/ion.h>

#include "Common.h"
#include "MessageQueue.h"
#include "Semaphore.h"
#include "CameraProperties.h"
#include "SensorListener.h"

//temporarily define format here
#define HAL_PIXEL_FORMAT_TI_NV12 0x100
#define HAL_PIXEL_FORMAT_TI_Y8 0x103
#define HAL_PIXEL_FORMAT_TI_Y16 0x104
#define HAL_PIXEL_FORMAT_TI_UYVY 0x105

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

#define FRAME_RATE_HIGH_HD 60

#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_HW_TEXTURE | \
                             GRALLOC_USAGE_HW_RENDER | \
                             GRALLOC_USAGE_SW_READ_RARELY | \
                             GRALLOC_USAGE_SW_WRITE_NEVER

//Enables Absolute PPM measurements in logcat
#define PPM_INSTRUMENTATION_ABS 1

#define LOCK_BUFFER_TRIES 5
#define HAL_PIXEL_FORMAT_NV12 0x100

#define OP_STR_SIZE 100

#define NONNEG_ASSIGN(x,y) \
    if(x > -1) \
        y = x

#define CAMHAL_SIZE_OF_ARRAY(x) static_cast<int>(sizeof(x)/sizeof(x[0]))

namespace Ti {
namespace Camera {

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
extern const char * const kRawImagesOutputDirPath;
extern const char * const kYuvImagesOutputDirPath;
#endif
#define V4L_CAMERA_NAME_USB     "USBCAMERA"
#define OMX_CAMERA_NAME_OV      "OV5640"
#define OMX_CAMERA_NAME_SONY    "IMX060"


///Forward declarations
class CameraHal;
class CameraFrame;
class CameraHalEvent;
class DisplayFrame;

class FpsRange {
public:
    static int compare(const FpsRange * left, const FpsRange * right);

    FpsRange(int min, int max);
    FpsRange();

    bool operator==(const FpsRange & fpsRange) const;

    bool isNull() const;
    bool isFixed() const;

    int min() const;
    int max() const;

private:
    int mMin;
    int mMax;
};


inline int FpsRange::compare(const FpsRange * const left, const FpsRange * const right) {
    if ( left->max() < right->max() ) {
        return -1;
    }

    if ( left->max() > right->max() ) {
        return 1;
    }

    if ( left->min() < right->min() ) {
        return -1;
    }

    if ( left->min() > right->min() ) {
        return 1;
    }

    return 0;
}

inline FpsRange::FpsRange(const int min, const int max) : mMin(min), mMax(max) {}

inline FpsRange::FpsRange() : mMin(-1), mMax(-1) {}

inline bool FpsRange::operator==(const FpsRange & fpsRange) const {
    return mMin == fpsRange.mMin && mMax == fpsRange.mMax;
}

inline bool FpsRange::isNull() const {
    return mMin == -1 || mMax == -1;
}

inline bool FpsRange::isFixed() const {
    return mMin == mMax;
}

inline int FpsRange::min() const { return mMin; }

inline int FpsRange::max() const { return mMax; }

class CameraArea : public android::RefBase
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

    bool compare(const android::sp<CameraArea> &area);

    static status_t parseAreas(const char *area,
                               size_t areaLength,
                               android::Vector< android::sp<CameraArea> > &areas);

    static status_t checkArea(ssize_t top,
                              ssize_t left,
                              ssize_t bottom,
                              ssize_t right,
                              ssize_t weight);

    static bool areAreasDifferent(android::Vector< android::sp<CameraArea> > &, android::Vector< android::sp<CameraArea> > &);

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

class CameraMetadataResult : public android::RefBase
{
public:

#ifdef OMAP_ENHANCEMENT_CPCAM
    CameraMetadataResult(camera_memory_t * extMeta) : mExtendedMetadata(extMeta) {
        mMetadata.faces = NULL;
        mMetadata.number_of_faces = 0;
#ifdef OMAP_ENHANCEMENT
        mMetadata.analog_gain = 0;
        mMetadata.exposure_time = 0;
#endif
    };
#endif

    CameraMetadataResult() {
        mMetadata.faces = NULL;
        mMetadata.number_of_faces = 0;
#ifdef OMAP_ENHANCEMENT_CPCAM
        mMetadata.analog_gain = 0;
        mMetadata.exposure_time = 0;
#endif

#ifdef OMAP_ENHANCEMENT_CPCAM
        mExtendedMetadata = NULL;
#endif
   }

    virtual ~CameraMetadataResult() {
        if ( NULL != mMetadata.faces ) {
            free(mMetadata.faces);
        }
#ifdef OMAP_ENHANCEMENT_CPCAM
        if ( NULL != mExtendedMetadata ) {
            mExtendedMetadata->release(mExtendedMetadata);
        }
#endif
    }

    camera_frame_metadata_t *getMetadataResult() { return &mMetadata; };

#ifdef OMAP_ENHANCEMENT_CPCAM
    camera_memory_t *getExtendedMetadata() { return mExtendedMetadata; };
#endif

    static const ssize_t TOP = -1000;
    static const ssize_t LEFT = -1000;
    static const ssize_t BOTTOM = 1000;
    static const ssize_t RIGHT = 1000;
    static const ssize_t INVALID_DATA = -2000;

private:

    camera_frame_metadata_t mMetadata;
#ifdef OMAP_ENHANCEMENT_CPCAM
    camera_memory_t *mExtendedMetadata;
#endif
};

typedef enum {
    CAMERA_BUFFER_NONE = 0,
    CAMERA_BUFFER_GRALLOC,
    CAMERA_BUFFER_ANW,
    CAMERA_BUFFER_MEMORY,
    CAMERA_BUFFER_ION
} CameraBufferType;

typedef struct _CameraBuffer {
    CameraBufferType type;
    /* opaque is the generic drop-in replacement for the pointers
     * that were used previously */
    void *opaque;

    /* opaque has different meanings depending on the buffer type:
     *   GRALLOC - gralloc_handle_t
     *   ANW - a pointer to the buffer_handle_t (which corresponds to
     *         the ANativeWindowBuffer *)
     *   MEMORY - address of allocated memory
     *   ION - address of mapped ion allocation
     *
     * FIXME opaque should be split into several fields:
     *   - handle/pointer we got from the allocator
     *   - handle/value we pass to OMX
     *   - pointer to mapped memory (if the buffer is mapped)
     */

    /* mapped holds ptr to mapped memory in userspace */
    void *mapped;

    /* These are specific to ION buffers */
    struct ion_handle * ion_handle;
    int ion_fd;
    int fd;
    size_t size;
    int index;

    /* These describe the camera buffer */
    int width;
    int stride;
    int height;
    const char *format;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    struct timeval ppmStamp;

#endif

    /* These are for buffers which include borders */
    int offset; // where valid data starts
    int actual_size; // size of the entire buffer with borders
} CameraBuffer;

void * camera_buffer_get_omx_ptr (CameraBuffer *buffer);

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
            REPROCESS_INPUT_FRAME = 0x400,
            ALL_FRAMES = 0xFFFF   ///Maximum of 16 frame types supported
        };

    enum FrameQuirks
    {
        ENCODE_RAW_YUV422I_TO_JPEG = 0x1 << 0,
        HAS_EXIF_DATA = 0x1 << 1,
        FORMAT_YUV422I_YUYV = 0x1 << 2,
        FORMAT_YUV422I_UYVY = 0x1 << 3,
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
    mQuirks(0)
    {
      mYuv[0] = 0;
      mYuv[1] = 0;

#ifdef OMAP_ENHANCEMENT_CPCAM
        mMetaData = 0;
#endif
    }

    void *mCookie;
    void *mCookie2;
    CameraBuffer *mBuffer;
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
#ifdef OMAP_ENHANCEMENT_CPCAM
    android::sp<CameraMetadataResult> mMetaData;
#endif
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
        EVENT_METADATA = 0x10,
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

    typedef android::sp<CameraMetadataResult> MetaEventData;

    class CameraHalEventData : public android::RefBase{

    public:

        CameraHalEvent::FocusEventData focusEvent;
        CameraHalEvent::ZoomEventData zoomEvent;
        CameraHalEvent::ShutterEventData shutterEvent;
        CameraHalEvent::MetaEventData metadataEvent;
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
    android::sp<CameraHalEventData> mEventData;

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

class ErrorNotifier : public virtual android::RefBase
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
    virtual void returnFrame(CameraBuffer* frameBuf, CameraFrame::FrameType frameType) = 0;
    virtual void addFramePointers(CameraBuffer *frameBuf, void *buf) = 0;
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
    int returnFrame(CameraBuffer *frameBuf, CameraFrame::FrameType frameType);
    void addFramePointers(CameraBuffer *frameBuf, void *buf);
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
    virtual CameraBuffer * allocateBufferList(int width, int height, const char* format, int &bytes, int numBufs) = 0;

    // gets a buffer list from BufferProvider when buffers are sent from external source and already pre-allocated
    // only call this function for an input source into CameraHal. If buffers are not from a pre-allocated source
    // this function will return NULL and numBufs of -1
    virtual CameraBuffer *getBufferList(int *numBufs) = 0;

    //additional methods used for memory mapping
    virtual uint32_t * getOffsets() = 0;
    virtual int getFd() = 0;
    virtual CameraBuffer * getBuffers(bool reset = false) { return NULL; }
    virtual unsigned int getSize() {return 0; }
    virtual int getBufferCount() {return -1; }

    virtual int freeBufferList(CameraBuffer * buf) = 0;

    virtual ~BufferProvider() {}
};

/**
  * Class for handling data and notify callbacks to application
  */
class   AppCallbackNotifier: public ErrorNotifier , public virtual android::RefBase
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

    status_t startPreviewCallbacks(android::CameraParameters &params, CameraBuffer *buffers, uint32_t *offsets, int fd, size_t length, size_t count);
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
    status_t initSharedVideoBuffers(CameraBuffer *buffers, uint32_t *offsets, int fd, size_t length, size_t count, CameraBuffer *vidBufs);
    status_t releaseRecordingFrame(const void *opaque);

    status_t useMetaDataBufferMode(bool enable);

    void EncoderDoneCb(void*, void*, CameraFrame::FrameType type, void* cookie1, void* cookie2, void *cookie3);

    void useVideoBuffers(bool useVideoBuffers);

    bool getUesVideoBuffers();
    void setVideoRes(int width, int height);

    void flushEventQueue();

    //Internal class definitions
    class NotificationThread : public android::Thread {
        AppCallbackNotifier* mAppCallbackNotifier;
        Utils::MessageQueue mNotificationThreadQ;
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

        Utils::MessageQueue &msgQ() { return mNotificationThreadQ;}
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
    size_t calculateBufferSize(size_t width, size_t height, const char *pixelFormat);
    const char* getContstantForPixelFormat(const char *pixelFormat);

private:
    mutable android::Mutex mLock;
    mutable android::Mutex mBurstLock;
    CameraHal* mCameraHal;
    camera_notify_callback mNotifyCb;
    camera_data_callback   mDataCb;
    camera_data_timestamp_callback mDataCbTimestamp;
    camera_request_memory mRequestMemory;
    void *mCallbackCookie;

    //Keeps Video MemoryHeaps and Buffers within
    //these objects
    android::KeyedVector<unsigned int, unsigned int> mVideoHeaps;
    android::KeyedVector<unsigned int, unsigned int> mVideoBuffers;
    android::KeyedVector<void *, CameraBuffer *> mVideoMap;

    //Keeps list of Gralloc handles and associated Video Metadata Buffers
    android::KeyedVector<void *, camera_memory_t *> mVideoMetadataBufferMemoryMap;
    android::KeyedVector<void *, CameraBuffer *> mVideoMetadataBufferReverseMap;

    bool mBufferReleased;

    android::sp< NotificationThread> mNotificationThread;
    EventProvider *mEventProvider;
    FrameProvider *mFrameProvider;
    Utils::MessageQueue mEventQ;
    Utils::MessageQueue mFrameQ;
    NotifierState mNotifierState;

    bool mPreviewing;
    camera_memory_t* mPreviewMemory;
    CameraBuffer mPreviewBuffers[MAX_BUFFERS];
    int mPreviewBufCount;
    int mPreviewWidth;
    int mPreviewHeight;
    int mPreviewStride;
    const char *mPreviewPixelFormat;
    android::KeyedVector<unsigned int, android::sp<android::MemoryHeapBase> > mSharedPreviewHeaps;
    android::KeyedVector<unsigned int, android::sp<android::MemoryBase> > mSharedPreviewBuffers;

    //Burst mode active
    bool mBurst;
    mutable android::Mutex mRecordingLock;
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
class MemoryManager : public BufferProvider, public virtual android::RefBase
{
public:
    MemoryManager();
    ~MemoryManager();

    status_t initialize();

    int setErrorHandler(ErrorNotifier *errorNotifier);
    virtual CameraBuffer * allocateBufferList(int width, int height, const char* format, int &bytes, int numBufs);
    virtual CameraBuffer *getBufferList(int *numBufs);
    virtual uint32_t * getOffsets();
    virtual int getFd() ;
    virtual int freeBufferList(CameraBuffer * buflist);

private:
    android::sp<ErrorNotifier> mErrorNotifier;
    int mIonFd;
};




/**
  * CameraAdapter interface class
  * Concrete classes derive from this class and provide implementations based on the specific camera h/w interface
  */

class CameraAdapter: public FrameNotifier, public virtual android::RefBase
{
protected:
    enum AdapterActiveStates {
        INTIALIZED_ACTIVE =         1 << 0,
        LOADED_PREVIEW_ACTIVE =     1 << 1,
        PREVIEW_ACTIVE =            1 << 2,
        LOADED_CAPTURE_ACTIVE =     1 << 3,
        CAPTURE_ACTIVE =            1 << 4,
        BRACKETING_ACTIVE =         1 << 5,
        AF_ACTIVE =                 1 << 6,
        ZOOM_ACTIVE =               1 << 7,
        VIDEO_ACTIVE =              1 << 8,
        LOADED_REPROCESS_ACTIVE =   1 << 9,
        REPROCESS_ACTIVE =          1 << 10,
    };
public:
    typedef struct
        {
         CameraBuffer *mBuffers;
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
        CAMERA_USE_BUFFERS_VIDEO_CAPTURE            = 25,
#ifdef OMAP_ENHANCEMENT_CPCAM
        CAMERA_USE_BUFFERS_REPROCESS                = 26,
        CAMERA_START_REPROCESS                      = 27,
#endif
#ifdef OMAP_ENHANCEMENT_VTC
        CAMERA_SETUP_TUNNEL                         = 28,
        CAMERA_DESTROY_TUNNEL                       = 29,
#endif
        CAMERA_PREVIEW_INITIALIZATION               = 30,
        };

    enum CameraMode
        {
        CAMERA_PREVIEW,
        CAMERA_IMAGE_CAPTURE,
        CAMERA_VIDEO,
        CAMERA_MEASUREMENT,
        CAMERA_REPROCESS,
        };

    enum AdapterState {
        INTIALIZED_STATE                = INTIALIZED_ACTIVE,
        LOADED_PREVIEW_STATE            = LOADED_PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        PREVIEW_STATE                   = PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        LOADED_CAPTURE_STATE            = LOADED_CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        CAPTURE_STATE                   = CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        BRACKETING_STATE                = BRACKETING_ACTIVE | CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE ,
        AF_STATE                        = AF_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        ZOOM_STATE                      = ZOOM_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_STATE                     = VIDEO_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_AF_STATE                  = VIDEO_ACTIVE | AF_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_ZOOM_STATE                = VIDEO_ACTIVE | ZOOM_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_LOADED_CAPTURE_STATE      = VIDEO_ACTIVE | LOADED_CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_CAPTURE_STATE             = VIDEO_ACTIVE | CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        AF_ZOOM_STATE                   = AF_ACTIVE | ZOOM_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        BRACKETING_ZOOM_STATE           = BRACKETING_ACTIVE | ZOOM_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        LOADED_REPROCESS_STATE          = LOADED_REPROCESS_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        LOADED_REPROCESS_CAPTURE_STATE  = LOADED_REPROCESS_ACTIVE | LOADED_CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        REPROCESS_STATE                 = REPROCESS_ACTIVE | CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
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
    virtual void returnFrame(CameraBuffer* frameBuf, CameraFrame::FrameType frameType) = 0;
    virtual void addFramePointers(CameraBuffer *frameBuf, void *buf) = 0;
    virtual void removeFramePointers() = 0;

    //APIs to configure Camera adapter and get the current parameter set
    virtual int setParameters(const android::CameraParameters& params) = 0;
    virtual void getParameters(android::CameraParameters& params) = 0;

    //Registers callback for returning image buffers back to CameraHAL
    virtual int registerImageReleaseCallback(release_image_buffers_callback callback, void *user_data) = 0;

    //Registers callback, which signals a completed image capture
    virtual int registerEndCaptureCallback(end_image_capture_callback callback, void *user_data) = 0;

    //API to send a command to the camera
    virtual status_t sendCommand(CameraCommands operation, int value1=0, int value2=0, int value3=0, int value4=0) = 0;

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

    virtual status_t setSharedAllocator(camera_request_memory shmem_alloc) = 0;

protected:
    //The first two methods will try to switch the adapter state.
    //Every call to setState() should be followed by a corresponding
    //call to commitState(). If the state switch fails, then it will
    //get reset to the previous state via rollbackState().
    virtual status_t setState(CameraCommands operation) = 0;
    virtual status_t commitState() = 0;
    virtual status_t rollbackState() = 0;
};

class DisplayAdapter : public BufferProvider, public virtual android::RefBase
{
public:
    DisplayAdapter();

#ifdef OMAP_ENHANCEMENT
    preview_stream_extended_ops_t * extendedOps() const {
        return mExtendedOps;
    }

    void setExtendedOps(preview_stream_extended_ops_t * extendedOps);
#endif

    ///Initializes the display adapter creates any resources required
    virtual int initialize() = 0;

    virtual int setPreviewWindow(struct preview_stream_ops *window) = 0;
    virtual int setFrameProvider(FrameNotifier *frameProvider) = 0;
    virtual int setErrorHandler(ErrorNotifier *errorNotifier) = 0;
    virtual int enableDisplay(int width, int height, struct timeval *refTime = NULL) = 0;
    virtual int disableDisplay(bool cancel_buffer = true) = 0;
    //Used for Snapshot review temp. pause
    virtual int pauseDisplay(bool pause) = 0;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
    //Used for shot to snapshot measurement
    virtual int setSnapshotTimeRef(struct timeval *refTime = NULL) = 0;
#endif

    virtual bool supportsExternalBuffering() = 0;

    // Get max queueable buffers display supports
    // This function should only be called after
    // allocateBufferList
    virtual status_t maxQueueableBuffers(unsigned int& queueable) = 0;

    // Get min buffers display needs at any given time
    virtual status_t minUndequeueableBuffers(int& unqueueable) = 0;

    // Given a vector of DisplayAdapters find the one corresponding to str
    virtual bool match(const char * str) { return false; }

private:
#ifdef OMAP_ENHANCEMENT
    preview_stream_extended_ops_t * mExtendedOps;
#endif
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
    static const int NO_BUFFERS_IMAGE_CAPTURE_SYSTEM_HEAP;
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
     * Set preview mode related initialization.
     * Only used when slice based processing is enabled.
     */
    int    cameraPreviewInitialization();

    /**
     * Only used if overlays are used for camera preview.
     */
    int setPreviewWindow(struct preview_stream_ops *window);

#ifdef OMAP_ENHANCEMENT_CPCAM
    void setExtendedPreviewStreamOps(preview_stream_extended_ops_t *ops);

    /**
     * Set a tap-in or tap-out point.
     */
    int setBufferSource(struct preview_stream_ops *tapin, struct preview_stream_ops *tapout);
#endif

    /**
     * Release a tap-in or tap-out point.
     */
    int releaseBufferSource(struct preview_stream_ops *tapin, struct preview_stream_ops *tapout);

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
    int    takePicture(const char* params);

    /**
     * Cancel a picture that was started with takePicture.  Calling this
     * method when no picture is being taken is a no-op.
     */
    int    cancelPicture();

    /** Set the camera parameters. */
    int    setParameters(const char* params);
    int    setParameters(const android::CameraParameters& params);

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

#ifdef OMAP_ENHANCEMENT_CPCAM
    /**
     * start a reprocessing operation.
     */
    int    reprocess(const char* params);

    /**
     * cancels current reprocessing operation
     */
    int    cancel_reprocess();
#endif

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

    static const char* getPixelFormatConstant(const char* parameters_format);
    static size_t calculateBufferSize(const char* parameters_format, int width, int height);
    static void getXYFromOffset(unsigned int *x, unsigned int *y,
                                unsigned int offset, unsigned int stride,
                                const char* format);
    static unsigned int getBPP(const char* format);

/*--------------------Internal Member functions - Private---------------------------------*/
private:

    /** @name internalFunctionsPrivate */
    //@{

    /**  Set the camera parameters specific to Video Recording. */
    bool        setVideoModeParameters(const android::CameraParameters&);

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
    status_t allocImageBufs(unsigned int width, unsigned int height, size_t length,
                            const char* previewFormat, unsigned int bufferCount);

    /** Allocate Raw buffers */
    status_t allocRawBufs(int width, int height, const char* previewFormat, int bufferCount);

    /** Free preview buffers */
    status_t freePreviewBufs();

    /** Free video bufs */
    status_t freeVideoBufs(CameraBuffer *bufs);

    /** Free RAW bufs */
    status_t freeRawBufs();

    //Check if a given resolution is supported by the current camera
    //instance
    bool isResolutionValid(unsigned int width, unsigned int height, const char *supportedResolutions);

    //Check if a given variable frame rate range is supported by the current camera
    //instance
    bool isFpsRangeValid(int fpsMin, int fpsMax, const char *supportedFpsRanges);

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

    void getPreferredPreviewRes(int *width, int *height);
    void resetPreviewRes(android::CameraParameters *params);

    // Internal __takePicture function - used in public takePicture() and reprocess()
    int   __takePicture(const char* params, struct timeval *captureStart = NULL);
    //@}

    status_t setTapoutLocked(struct preview_stream_ops *out);
    status_t releaseTapoutLocked(struct preview_stream_ops *out);
    status_t setTapinLocked(struct preview_stream_ops *in);
    status_t releaseTapinLocked(struct preview_stream_ops *in);
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
    android::sp<AppCallbackNotifier> mAppCallbackNotifier;
    android::sp<DisplayAdapter> mDisplayAdapter;
    android::sp<MemoryManager> mMemoryManager;

    android::Vector< android::sp<DisplayAdapter> > mOutAdapters;
    android::Vector< android::sp<DisplayAdapter> > mInAdapters;

    // TODO(XXX): Even though we support user setting multiple BufferSourceAdapters now
    // only one tap in surface and one tap out surface is supported at a time.
    android::sp<DisplayAdapter> mBufferSourceAdapter_In;
    android::sp<DisplayAdapter> mBufferSourceAdapter_Out;

#ifdef OMAP_ENHANCEMENT
    preview_stream_extended_ops_t * mExtendedPreviewStreamOps;
#endif

    android::sp<android::IMemoryHeap> mPictureHeap;

    int* mGrallocHandles;
    bool mFpsRangeChangedByApp;


    int mRawWidth;
    int mRawHeight;
    bool mRawCapture;


///static member vars

    static const int SW_SCALING_FPS_LIMIT;

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

#ifdef OMAP_ENHANCEMENT_VTC
    bool mTunnelSetup;
    bool mVTCUseCase;
#endif

    //Index of current camera adapter
    int mCameraIndex;

    mutable android::Mutex mLock;

    android::sp<SensorListener> mSensorListener;

    void* mCameraAdapterHandle;

    android::CameraParameters mParameters;
    bool mPreviewRunning;
    bool mPreviewStateOld;
    bool mRecordingEnabled;
    EventProvider *mEventProvider;

    CameraBuffer *mPreviewDataBuffers;
    uint32_t *mPreviewDataOffsets;
    int mPreviewDataFd;
    int mPreviewDataLength;
    CameraBuffer *mImageBuffers;
    uint32_t *mImageOffsets;
    int mImageFd;
    int mImageLength;
    unsigned int mImageCount;
    CameraBuffer *mPreviewBuffers;
    uint32_t *mPreviewOffsets;
    int mPreviewLength;
    int mPreviewFd;
    CameraBuffer *mVideoBuffers;
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
    bool mPreviewInitializationDone;

    bool mSetPreviewWindowCalled;

    uint32_t mPreviewWidth;
    uint32_t mPreviewHeight;
    int32_t mMaxZoomSupported;

    int mVideoWidth;
    int mVideoHeight;

    android::String8 mCapModeBackup;
};

} // namespace Camera
} // namespace Ti

#endif
