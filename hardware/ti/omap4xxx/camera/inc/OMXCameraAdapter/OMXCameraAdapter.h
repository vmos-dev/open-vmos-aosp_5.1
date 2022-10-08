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



#ifndef OMX_CAMERA_ADAPTER_H
#define OMX_CAMERA_ADAPTER_H

#include "CameraHal.h"
#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_CoreExt.h"
#include "OMX_IVCommon.h"
#include "OMX_Component.h"
#include "OMX_Index.h"
#include "OMX_IndexExt.h"
#include "OMX_TI_Index.h"
#include "OMX_TI_IVCommon.h"
#include "OMX_TI_Common.h"
#include "OMX_TI_Image.h"
#include "General3A_Settings.h"
#include "OMXSceneModeTables.h"

#include "BaseCameraAdapter.h"
#include "Encoder_libjpeg.h"
#include "DebugUtils.h"


extern "C"
{
#include "timm_osal_error.h"
#include "timm_osal_events.h"
#include "timm_osal_trace.h"
#include "timm_osal_semaphores.h"
}


namespace android {

#define Q16_OFFSET                  16

#define OMX_CMD_TIMEOUT             3000000  //3 sec.
#define OMX_CAPTURE_TIMEOUT         5000000  //5 sec.

#define FOCUS_THRESHOLD             5 //[s.]

#define MIN_JPEG_QUALITY            1
#define MAX_JPEG_QUALITY            100
#define EXP_BRACKET_RANGE           10

#define FOCUS_DIST_SIZE             100
#define FOCUS_DIST_BUFFER_SIZE      500

#define TOUCH_DATA_SIZE             200
#define DEFAULT_THUMB_WIDTH         160
#define DEFAULT_THUMB_HEIGHT        120
#define FRAME_RATE_FULL_HD          27
#define ZOOM_STAGES                 61

#define FACE_DETECTION_BUFFER_SIZE  0x1000
#define MAX_NUM_FACES_SUPPORTED     35

#define EXIF_MODEL_SIZE             100
#define EXIF_MAKE_SIZE              100
#define EXIF_DATE_TIME_SIZE         20

#define GPS_MIN_DIV                 60
#define GPS_SEC_DIV                 60
#define GPS_SEC_ACCURACY            1000
#define GPS_TIMESTAMP_SIZE          6
#define GPS_DATESTAMP_SIZE          11
#define GPS_REF_SIZE                2
#define GPS_MAPDATUM_SIZE           100
#define GPS_PROCESSING_SIZE         100
#define GPS_VERSION_SIZE            4
#define GPS_NORTH_REF               "N"
#define GPS_SOUTH_REF               "S"
#define GPS_EAST_REF                "E"
#define GPS_WEST_REF                "W"

/* Default portstartnumber of Camera component */
#define OMX_CAMERA_DEFAULT_START_PORT_NUM 0

/* Define number of ports for differt domains */
#define OMX_CAMERA_PORT_OTHER_NUM 1
#define OMX_CAMERA_PORT_VIDEO_NUM 4
#define OMX_CAMERA_PORT_IMAGE_NUM 1
#define OMX_CAMERA_PORT_AUDIO_NUM 0
#define OMX_CAMERA_NUM_PORTS (OMX_CAMERA_PORT_OTHER_NUM + OMX_CAMERA_PORT_VIDEO_NUM + OMX_CAMERA_PORT_IMAGE_NUM + OMX_CAMERA_PORT_AUDIO_NUM)

/* Define start port number for differt domains */
#define OMX_CAMERA_PORT_OTHER_START OMX_CAMERA_DEFAULT_START_PORT_NUM
#define OMX_CAMERA_PORT_VIDEO_START (OMX_CAMERA_PORT_OTHER_START + OMX_CAMERA_PORT_OTHER_NUM)
#define OMX_CAMERA_PORT_IMAGE_START (OMX_CAMERA_PORT_VIDEO_START + OMX_CAMERA_PORT_VIDEO_NUM)
#define OMX_CAMERA_PORT_AUDIO_START (OMX_CAMERA_PORT_IMAGE_START + OMX_CAMERA_PORT_IMAGE_NUM)

/* Port index for camera component */
#define OMX_CAMERA_PORT_OTHER_IN (OMX_CAMERA_PORT_OTHER_START + 0)
#define OMX_CAMERA_PORT_VIDEO_IN_VIDEO (OMX_CAMERA_PORT_VIDEO_START + 0)
#define OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW (OMX_CAMERA_PORT_VIDEO_START + 1)
#define OMX_CAMERA_PORT_VIDEO_OUT_VIDEO (OMX_CAMERA_PORT_VIDEO_START + 2)
#define OMX_CAMERA_PORT_VIDEO_OUT_MEASUREMENT (OMX_CAMERA_PORT_VIDEO_START + 3)
#define OMX_CAMERA_PORT_IMAGE_OUT_IMAGE (OMX_CAMERA_PORT_IMAGE_START + 0)


#define OMX_INIT_STRUCT(_s_, _name_)	\
    memset(&(_s_), 0x0, sizeof(_name_));	\
    (_s_).nSize = sizeof(_name_);		\
    (_s_).nVersion.s.nVersionMajor = 0x1;	\
    (_s_).nVersion.s.nVersionMinor = 0x1;	\
    (_s_).nVersion.s.nRevision = 0x0;		\
    (_s_).nVersion.s.nStep = 0x0

#define OMX_INIT_STRUCT_PTR(_s_, _name_)   \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 0x1;      \
    (_s_)->nVersion.s.nVersionMinor = 0x1;      \
    (_s_)->nVersion.s.nRevision = 0x0;          \
    (_s_)->nVersion.s.nStep = 0x0

#define GOTO_EXIT_IF(_CONDITION,_ERROR) {                                       \
    if ((_CONDITION)) {                                                         \
        eError = (_ERROR);                                                      \
        goto EXIT;                                                              \
    }                                                                           \
}

const int64_t kCameraBufferLatencyNs = 250000000LL; // 250 ms

///OMX Specific Functions
static OMX_ERRORTYPE OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_PTR pAppData,
                                        OMX_IN OMX_EVENTTYPE eEvent,
                                        OMX_IN OMX_U32 nData1,
                                        OMX_IN OMX_U32 nData2,
                                        OMX_IN OMX_PTR pEventData);

static OMX_ERRORTYPE OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_PTR pAppData,
                                        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_PTR pAppData,
                                        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader);

struct CapResolution {
    size_t width, height;
    const char *param;
};

struct CapPixelformat {
    OMX_COLOR_FORMATTYPE pixelformat;
    const char *param;
};

struct CapU32 {
    OMX_U32 num;
    const char *param;
};

struct CapU32Pair {
    OMX_U32 num1, num2;
    const char *param;
};
struct CapS32 {
    OMX_S32 num;
    const char *param;
};

typedef CapU32 CapFramerate;
typedef CapU32 CapISO;
typedef CapU32 CapSensorName;
typedef CapS32 CapZoom;
typedef CapS32 CapEVComp;

/**
  * Class which completely abstracts the camera hardware interaction from camera hal
  * TODO: Need to list down here, all the message types that will be supported by this class
                Need to implement BufferProvider interface to use AllocateBuffer of OMX if needed
  */
class OMXCameraAdapter : public BaseCameraAdapter
{
public:

    /*--------------------Constant declarations----------------------------------------*/
    static const int32_t MAX_NO_BUFFERS = 20;

    ///@remarks OMX Camera has six ports - buffer input, time input, preview, image, video, and meta data
    static const int MAX_NO_PORTS = 6;

    ///Five second timeout
    static const int CAMERA_ADAPTER_TIMEOUT = 5000*1000;

    enum OMXCameraEvents
        {
        CAMERA_PORT_ENABLE  = 0x1,
        CAMERA_PORT_FLUSH   = 0x2,
        CAMERA_PORT_DISABLE = 0x4,
        };

    enum CaptureMode
        {
        HIGH_SPEED = 1,
        HIGH_QUALITY = 2,
        VIDEO_MODE = 3,
        HIGH_QUALITY_ZSL = 4,
        };

    enum IPPMode
        {
        IPP_NULL = -1,
        IPP_NONE = 0,
        IPP_NSF,
        IPP_LDC,
        IPP_LDCNSF,
        };

    enum CodingMode
        {
        CodingNone = 0,
        CodingJPS,
        CodingMPO,
        CodingRAWJPEG,
        CodingRAWMPO,
        };

    enum Algorithm3A
        {
        WHITE_BALANCE_ALGO = 0x1,
        EXPOSURE_ALGO = 0x2,
        FOCUS_ALGO = 0x4,
        };

    enum AlgoPriority
        {
        FACE_PRIORITY = 0,
        REGION_PRIORITY,
        };

    enum BrightnessMode
        {
        BRIGHTNESS_OFF = 0,
        BRIGHTNESS_ON,
        BRIGHTNESS_AUTO,
        };

    enum CaptureSettingsFlags {
        SetFormat               = 1 << 0,
        SetThumb                = 1 << 1,
        SetExpBracket           = 1 << 2,
        SetQuality              = 1 << 3,
        SetRotation             = 1 << 4,
        ECaptureSettingMax,
        ECapturesettingsAll = ( ((ECaptureSettingMax -1 ) << 1) -1 ) /// all possible flags raised
    };

    class GPSData
    {
        public:
                int mLongDeg, mLongMin, mLongSec, mLongSecDiv;
                char mLongRef[GPS_REF_SIZE];
                bool mLongValid;
                int mLatDeg, mLatMin, mLatSec, mLatSecDiv;
                char mLatRef[GPS_REF_SIZE];
                bool mLatValid;
                int mAltitude;
                unsigned char mAltitudeRef;
                bool mAltitudeValid;
                char mMapDatum[GPS_MAPDATUM_SIZE];
                bool mMapDatumValid;
                char mVersionId[GPS_VERSION_SIZE];
                bool mVersionIdValid;
                char mProcMethod[GPS_PROCESSING_SIZE];
                bool mProcMethodValid;
                char mDatestamp[GPS_DATESTAMP_SIZE];
                bool mDatestampValid;
                uint32_t mTimeStampHour;
                uint32_t mTimeStampMin;
                uint32_t mTimeStampSec;
                bool mTimeStampValid;
    };

    class EXIFData
    {
        public:
            GPSData mGPSData;
            char mMake[EXIF_MODEL_SIZE];
            char mModel[EXIF_MAKE_SIZE];
            unsigned int mFocalNum, mFocalDen;
            bool mMakeValid;
            bool mModelValid;
    };

    ///Parameters specific to any port of the OMX Camera component
    class OMXCameraPortParameters
    {
        public:
            OMX_U32                         mHostBufaddr[MAX_NO_BUFFERS];
            OMX_BUFFERHEADERTYPE           *mBufferHeader[MAX_NO_BUFFERS];
            OMX_U32                         mWidth;
            OMX_U32                         mHeight;
            OMX_U32                         mStride;
            OMX_U8                          mNumBufs;

            // defines maximum number of buffers our of mNumBufs
            // queueable at given moment
            OMX_U8                          mMaxQueueable;

            OMX_U32                         mBufSize;
            OMX_COLOR_FORMATTYPE            mColorFormat;
            OMX_PARAM_VIDEONOISEFILTERTYPE  mVNFMode;
            OMX_PARAM_VIDEOYUVRANGETYPE     mYUVRange;
            OMX_CONFIG_BOOLEANTYPE          mVidStabParam;
            OMX_CONFIG_FRAMESTABTYPE        mVidStabConfig;
            OMX_U32                         mCapFrame;
            OMX_U32                         mFrameRate;
            OMX_S32                         mMinFrameRate;
            OMX_S32                         mMaxFrameRate;
            CameraFrame::FrameType mImageType;
    };

    ///Context of the OMX Camera component
    class OMXCameraAdapterComponentContext
    {
        public:
            OMX_HANDLETYPE              mHandleComp;
            OMX_U32                     mNumPorts;
            OMX_STATETYPE               mState ;
            OMX_U32                     mVideoPortIndex;
            OMX_U32                     mPrevPortIndex;
            OMX_U32                     mImagePortIndex;
            OMX_U32                     mMeasurementPortIndex;
            OMXCameraPortParameters     mCameraPortParams[MAX_NO_PORTS];
    };

public:

    OMXCameraAdapter(size_t sensor_index);
    ~OMXCameraAdapter();

    ///Initialzes the camera adapter creates any resources required
    virtual status_t initialize(CameraProperties::Properties*);

    //APIs to configure Camera adapter and get the current parameter set
    virtual status_t setParameters(const CameraParameters& params);
    virtual void getParameters(CameraParameters& params);

    // API
    virtual status_t UseBuffersPreview(void* bufArr, int num);

    //API to flush the buffers for preview
    status_t flushBuffers();

    // API
    virtual status_t setFormat(OMX_U32 port, OMXCameraPortParameters &cap);

    // Function to get and populate caps from handle
    static status_t getCaps(CameraProperties::Properties* props, OMX_HANDLETYPE handle);
    static const char* getLUTvalue_OMXtoHAL(int OMXValue, LUTtype LUT);
    static int getLUTvalue_HALtoOMX(const char * HalValue, LUTtype LUT);

 OMX_ERRORTYPE OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                    OMX_IN OMX_EVENTTYPE eEvent,
                                    OMX_IN OMX_U32 nData1,
                                    OMX_IN OMX_U32 nData2,
                                    OMX_IN OMX_PTR pEventData);

 OMX_ERRORTYPE OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

 OMX_ERRORTYPE OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                    OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader);

 static OMX_ERRORTYPE OMXCameraGetHandle(OMX_HANDLETYPE *handle, OMX_PTR pAppData=NULL);

protected:

    //Parent class method implementation
    virtual status_t takePicture();
    virtual status_t stopImageCapture();
    virtual status_t startBracketing(int range);
    virtual status_t stopBracketing();
    virtual status_t autoFocus();
    virtual status_t cancelAutoFocus();
    virtual status_t startSmoothZoom(int targetIdx);
    virtual status_t stopSmoothZoom();
    virtual status_t startVideoCapture();
    virtual status_t stopVideoCapture();
    virtual status_t startPreview();
    virtual status_t stopPreview();
    virtual status_t useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable);
    virtual status_t fillThisBuffer(void* frameBuf, CameraFrame::FrameType frameType);
    virtual status_t getFrameSize(size_t &width, size_t &height);
    virtual status_t getPictureBufferSize(size_t &length, size_t bufferCount);
    virtual status_t getFrameDataSize(size_t &dataFrameSize, size_t bufferCount);
    virtual status_t startFaceDetection();
    virtual status_t stopFaceDetection();
    virtual status_t switchToExecuting();
    virtual void onOrientationEvent(uint32_t orientation, uint32_t tilt);

private:

    status_t doSwitchToExecuting();

    void performCleanupAfterError();

    status_t switchToLoaded();

    OMXCameraPortParameters *getPortParams(CameraFrame::FrameType frameType);

    OMX_ERRORTYPE SignalEvent(OMX_IN OMX_HANDLETYPE hComponent,
                                                  OMX_IN OMX_EVENTTYPE eEvent,
                                                  OMX_IN OMX_U32 nData1,
                                                  OMX_IN OMX_U32 nData2,
                                                  OMX_IN OMX_PTR pEventData);
    OMX_ERRORTYPE RemoveEvent(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_EVENTTYPE eEvent,
                              OMX_IN OMX_U32 nData1,
                              OMX_IN OMX_U32 nData2,
                              OMX_IN OMX_PTR pEventData);

    status_t RegisterForEvent(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_EVENTTYPE eEvent,
                                          OMX_IN OMX_U32 nData1,
                                          OMX_IN OMX_U32 nData2,
                                          OMX_IN Semaphore &semaphore);

    status_t setPictureRotation(unsigned int degree);
    status_t setSensorOrientation(unsigned int degree);
    status_t setImageQuality(unsigned int quality);
    status_t setThumbnailParams(unsigned int width, unsigned int height, unsigned int quality);

    //EXIF
    status_t setParametersEXIF(const CameraParameters &params,
                               BaseCameraAdapter::AdapterState state);
    status_t convertGPSCoord(double coord, int &deg, int &min, int &sec, int &secDivisor);
    status_t setupEXIF();
    status_t setupEXIF_libjpeg(ExifElementsTable*, OMX_TI_ANCILLARYDATATYPE*,
                               OMX_TI_WHITEBALANCERESULTTYPE*);

    //Focus functionality
    status_t doAutoFocus();
    status_t stopAutoFocus();
    status_t checkFocus(OMX_PARAM_FOCUSSTATUSTYPE *eFocusStatus);
    status_t returnFocusStatus(bool timeoutReached);
    status_t getFocusMode(OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE &focusMode);
    void handleFocusCallback();


    //Focus distances
    status_t setParametersFocus(const CameraParameters &params,
                                BaseCameraAdapter::AdapterState state);
    status_t addFocusDistances(OMX_U32 &near,
                               OMX_U32 &optimal,
                               OMX_U32 &far,
                               CameraParameters& params);
    status_t encodeFocusDistance(OMX_U32 dist, char *buffer, size_t length);
    status_t getFocusDistances(OMX_U32 &near,OMX_U32 &optimal, OMX_U32 &far);

    //VSTAB and VNF Functionality
    status_t enableVideoNoiseFilter(bool enable);
    status_t enableVideoStabilization(bool enable);

    //Digital zoom
    status_t setParametersZoom(const CameraParameters &params,
                               BaseCameraAdapter::AdapterState state);
    status_t doZoom(int index);
    status_t advanceZoom();

    //3A related parameters
    status_t setParameters3A(const CameraParameters &params,
                             BaseCameraAdapter::AdapterState state);

    // scene modes
    status_t setScene(Gen3A_settings& Gen3A);
    // returns pointer to SceneModesEntry from the LUT for camera given 'name' and 'scene'
    static const SceneModesEntry* getSceneModeEntry(const char* name, OMX_SCENEMODETYPE scene);


    //Flash modes
    status_t setFlashMode(Gen3A_settings& Gen3A);
    status_t getFlashMode(Gen3A_settings& Gen3A);

    // Focus modes
    status_t setFocusMode(Gen3A_settings& Gen3A);
    status_t getFocusMode(Gen3A_settings& Gen3A);

    //Exposure Modes
    status_t setExposureMode(Gen3A_settings& Gen3A);
    status_t setEVCompensation(Gen3A_settings& Gen3A);
    status_t setWBMode(Gen3A_settings& Gen3A);
    status_t setFlicker(Gen3A_settings& Gen3A);
    status_t setBrightness(Gen3A_settings& Gen3A);
    status_t setContrast(Gen3A_settings& Gen3A);
    status_t setSharpness(Gen3A_settings& Gen3A);
    status_t setSaturation(Gen3A_settings& Gen3A);
    status_t setISO(Gen3A_settings& Gen3A);
    status_t setEffect(Gen3A_settings& Gen3A);
    status_t setMeteringAreas(Gen3A_settings& Gen3A);

    status_t getEVCompensation(Gen3A_settings& Gen3A);
    status_t getWBMode(Gen3A_settings& Gen3A);
    status_t getSharpness(Gen3A_settings& Gen3A);
    status_t getSaturation(Gen3A_settings& Gen3A);
    status_t getISO(Gen3A_settings& Gen3A);

    // 3A locks
    status_t setExposureLock(Gen3A_settings& Gen3A);
    status_t setFocusLock(Gen3A_settings& Gen3A);
    status_t setWhiteBalanceLock(Gen3A_settings& Gen3A);
    status_t set3ALock(OMX_BOOL toggleExp, OMX_BOOL toggleWb, OMX_BOOL toggleFocus);

    //API to set FrameRate using VFR interface
    status_t setVFramerate(OMX_U32 minFrameRate,OMX_U32 maxFrameRate);

    status_t setParametersAlgo(const CameraParameters &params,
                               BaseCameraAdapter::AdapterState state);

    //Noise filtering
    status_t setNSF(OMXCameraAdapter::IPPMode mode);

    //LDC
    status_t setLDC(OMXCameraAdapter::IPPMode mode);

    //GLBCE
    status_t setGLBCE(OMXCameraAdapter::BrightnessMode mode);

    //GBCE
    status_t setGBCE(OMXCameraAdapter::BrightnessMode mode);

    status_t printComponentVersion(OMX_HANDLETYPE handle);

    //Touch AF
    status_t setTouchFocus();

    //Face detection
    status_t setParametersFD(const CameraParameters &params,
                             BaseCameraAdapter::AdapterState state);
    status_t updateFocusDistances(CameraParameters &params);
    status_t setFaceDetection(bool enable, OMX_U32 orientation);
    status_t detectFaces(OMX_BUFFERHEADERTYPE* pBuffHeader,
                         sp<CameraFDResult> &result,
                         size_t previewWidth,
                         size_t previewHeight);
    status_t encodeFaceCoordinates(const OMX_FACEDETECTIONTYPE *faceData,
                                   camera_frame_metadata_t **pFaces,
                                   size_t previewWidth,
                                   size_t previewHeight);
    void pauseFaceDetection(bool pause);

    //3A Algorithms priority configuration
    status_t setAlgoPriority(AlgoPriority priority, Algorithm3A algo, bool enable);

    //Sensor overclocking
    status_t setSensorOverclock(bool enable);

    // Utility methods for OMX Capabilities
    static status_t insertCapabilities(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t encodeSizeCap(OMX_TI_CAPRESTYPE&, const CapResolution *, size_t, char *, size_t);
    static status_t encodeISOCap(OMX_U32, const CapISO*, size_t, char*, size_t);
    static size_t encodeZoomCap(OMX_S32, const CapZoom*, size_t, char*, size_t);
    static status_t encodeFramerateCap(OMX_U32, OMX_U32, const CapFramerate*, size_t, char*, size_t);
    static status_t encodeVFramerateCap(OMX_TI_CAPTYPE&, const CapU32Pair*, size_t, char*, char*, size_t);
    static status_t encodePixelformatCap(OMX_COLOR_FORMATTYPE,
                                         const CapPixelformat*,
                                         size_t,
                                         char*,
                                         size_t);
    static status_t insertImageSizes(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertPreviewSizes(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertThumbSizes(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertZoomStages(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertImageFormats(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertPreviewFormats(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertFramerates(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertVFramerates(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertEVs(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertISOModes(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertIPPModes(CameraProperties::Properties*, OMX_TI_CAPTYPE &);
    static status_t insertWBModes(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertEffects(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertExpModes(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertSceneModes(CameraProperties::Properties*, OMX_TI_CAPTYPE &);
    static status_t insertFocusModes(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertFlickerModes(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertFlashModes(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertSenMount(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertDefaults(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertLocks(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertAreas(CameraProperties::Properties*, OMX_TI_CAPTYPE&);
    static status_t insertVideoSizes(CameraProperties::Properties*, OMX_TI_CAPTYPE&);

    status_t setParametersCapture(const CameraParameters &params,
                                  BaseCameraAdapter::AdapterState state);

    //Exposure Bracketing
    status_t setExposureBracketing(int *evValues, size_t evCount, size_t frameCount);
    status_t parseExpRange(const char *rangeStr, int * expRange, size_t count, size_t &validEntries);

    //Temporal Bracketing
    status_t doBracketing(OMX_BUFFERHEADERTYPE *pBuffHeader, CameraFrame::FrameType typeOfFrame);
    status_t sendBracketFrames();

    // Image Capture Service
    status_t startImageCapture();
    status_t disableImagePort();

    //Shutter callback notifications
    status_t setShutterCallback(bool enabled);

    //Sets eithter HQ or HS mode and the frame count
    status_t setCaptureMode(OMXCameraAdapter::CaptureMode mode);
    status_t UseBuffersCapture(void* bufArr, int num);
    status_t UseBuffersPreviewData(void* bufArr, int num);

    //Used for calculation of the average frame rate during preview
    status_t recalculateFPS();

    //Helper method for initializing a CameFrame object
    status_t initCameraFrame(CameraFrame &frame, OMX_IN OMX_BUFFERHEADERTYPE *pBuffHeader, int typeOfFrame, OMXCameraPortParameters *port);

    //Sends the incoming OMX buffer header to subscribers
    status_t sendFrame(CameraFrame &frame);

    status_t sendCallBacks(CameraFrame frame, OMX_IN OMX_BUFFERHEADERTYPE *pBuffHeader, unsigned int mask, OMXCameraPortParameters *port);

    status_t apply3Asettings( Gen3A_settings& Gen3A );
    status_t init3AParams(Gen3A_settings &Gen3A);

    // AutoConvergence
    status_t setAutoConvergence(OMX_TI_AUTOCONVERGENCEMODETYPE pACMode, OMX_S32 pManualConverence);
    status_t getAutoConvergence(OMX_TI_AUTOCONVERGENCEMODETYPE *pACMode, OMX_S32 *pManualConverence);

    status_t setExtraData(bool enable, OMX_U32, OMX_EXT_EXTRADATATYPE);
    OMX_OTHER_EXTRADATATYPE *getExtradata(OMX_OTHER_EXTRADATATYPE *extraData, OMX_EXTRADATATYPE type);

    class CommandHandler : public Thread {
        public:
            CommandHandler(OMXCameraAdapter* ca)
                : Thread(false), mCameraAdapter(ca) { }

            virtual bool threadLoop() {
                bool ret;
                ret = Handler();
                return ret;
            }

            status_t put(TIUTILS::Message* msg){
                Mutex::Autolock lock(mLock);
                return mCommandMsgQ.put(msg);
            }

            void clearCommandQ()
                {
                Mutex::Autolock lock(mLock);
                mCommandMsgQ.clear();
                }

            enum {
                COMMAND_EXIT = -1,
                CAMERA_START_IMAGE_CAPTURE = 0,
                CAMERA_PERFORM_AUTOFOCUS = 1,
                CAMERA_SWITCH_TO_EXECUTING,
            };

        private:
            bool Handler();
            TIUTILS::MessageQueue mCommandMsgQ;
            OMXCameraAdapter* mCameraAdapter;
            Mutex mLock;
    };
    sp<CommandHandler> mCommandHandler;

public:

    class OMXCallbackHandler : public Thread {
        public:
        OMXCallbackHandler(OMXCameraAdapter* ca)
            : Thread(false), mCameraAdapter(ca) { }

        virtual bool threadLoop() {
            bool ret;
            ret = Handler();
            return ret;
        }

        status_t put(TIUTILS::Message* msg){
            Mutex::Autolock lock(mLock);
            return mCommandMsgQ.put(msg);
        }

        void clearCommandQ()
            {
            Mutex::Autolock lock(mLock);
            mCommandMsgQ.clear();
            }

        enum {
            COMMAND_EXIT = -1,
            CAMERA_FILL_BUFFER_DONE,
            CAMERA_FOCUS_STATUS,
        };

    private:
        bool Handler();
        TIUTILS::MessageQueue mCommandMsgQ;
        OMXCameraAdapter* mCameraAdapter;
        Mutex mLock;
    };

    sp<OMXCallbackHandler> mOMXCallbackHandler;

private:

    //AF callback
    status_t setFocusCallback(bool enabled);

    //OMX Capabilities data
    static const CapResolution mImageCapRes [];
    static const CapResolution mPreviewRes [];
    static const CapResolution mThumbRes [];
    static const CapPixelformat mPixelformats [];
    static const CapFramerate mFramerates [];
    static const CapU32 mSensorNames[] ;
    static const CapZoom mZoomStages [];
    static const CapEVComp mEVCompRanges [];
    static const CapISO mISOStages [];
    static const CapU32Pair mVarFramerates [];

    // OMX Camera defaults
    static const char DEFAULT_ANTIBANDING[];
    static const char DEFAULT_BRIGHTNESS[];
    static const char DEFAULT_CONTRAST[];
    static const char DEFAULT_EFFECT[];
    static const char DEFAULT_EV_COMPENSATION[];
    static const char DEFAULT_EV_STEP[];
    static const char DEFAULT_EXPOSURE_MODE[];
    static const char DEFAULT_FLASH_MODE[];
    static const char DEFAULT_FOCUS_MODE_PREFERRED[];
    static const char DEFAULT_FOCUS_MODE[];
    static const char DEFAULT_FRAMERATE_RANGE_IMAGE[];
    static const char DEFAULT_FRAMERATE_RANGE_VIDEO[];
    static const char DEFAULT_IPP[];
    static const char DEFAULT_GBCE[];
    static const char DEFAULT_ISO_MODE[];
    static const char DEFAULT_JPEG_QUALITY[];
    static const char DEFAULT_THUMBNAIL_QUALITY[];
    static const char DEFAULT_THUMBNAIL_SIZE[];
    static const char DEFAULT_PICTURE_FORMAT[];
    static const char DEFAULT_PICTURE_SIZE[];
    static const char DEFAULT_PREVIEW_FORMAT[];
    static const char DEFAULT_FRAMERATE[];
    static const char DEFAULT_PREVIEW_SIZE[];
    static const char DEFAULT_NUM_PREV_BUFS[];
    static const char DEFAULT_NUM_PIC_BUFS[];
    static const char DEFAULT_MAX_FOCUS_AREAS[];
    static const char DEFAULT_SATURATION[];
    static const char DEFAULT_SCENE_MODE[];
    static const char DEFAULT_SHARPNESS[];
    static const char DEFAULT_VSTAB[];
    static const char DEFAULT_VSTAB_SUPPORTED[];
    static const char DEFAULT_WB[];
    static const char DEFAULT_ZOOM[];
    static const char DEFAULT_MAX_FD_HW_FACES[];
    static const char DEFAULT_MAX_FD_SW_FACES[];
    static const char DEFAULT_AE_LOCK[];
    static const char DEFAULT_AWB_LOCK[];
    static const char DEFAULT_MAX_NUM_METERING_AREAS[];
    static const char DEFAULT_LOCK_SUPPORTED[];
    static const char DEFAULT_LOCK_UNSUPPORTED[];
    static const char DEFAULT_FOCAL_LENGTH_PRIMARY[];
    static const char DEFAULT_FOCAL_LENGTH_SECONDARY[];
    static const char DEFAULT_HOR_ANGLE[];
    static const char DEFAULT_VER_ANGLE[];
    static const char DEFAULT_VIDEO_SNAPSHOT_SUPPORTED[];
    static const char DEFAULT_VIDEO_SIZE[];
    static const char DEFAULT_PREFERRED_PREVIEW_SIZE_FOR_VIDEO[];
    static const size_t MAX_FOCUS_AREAS;

    OMX_VERSIONTYPE mCompRevision;

    //OMX Component UUID
    OMX_UUIDTYPE mCompUUID;

    //Current Focus distances
    char mFocusDistNear[FOCUS_DIST_SIZE];
    char mFocusDistOptimal[FOCUS_DIST_SIZE];
    char mFocusDistFar[FOCUS_DIST_SIZE];
    char mFocusDistBuffer[FOCUS_DIST_BUFFER_SIZE];

    // Current Focus areas
    Vector< sp<CameraArea> > mFocusAreas;
    mutable Mutex mFocusAreasLock;

    // Current Metering areas
    Vector< sp<CameraArea> > mMeteringAreas;
    mutable Mutex mMeteringAreasLock;

    CaptureMode mCapMode;
    size_t mBurstFrames;
    size_t mCapturedFrames;

    bool mMeasurementEnabled;

    //Exposure Bracketing
    int mExposureBracketingValues[EXP_BRACKET_RANGE];
    size_t mExposureBracketingValidEntries;

    mutable Mutex mFaceDetectionLock;
    //Face detection status
    bool mFaceDetectionRunning;
    bool mFaceDetectionPaused;
    bool mFDSwitchAlgoPriority;

    camera_face_t  faceDetectionLastOutput [MAX_NUM_FACES_SUPPORTED];
    int faceDetectionNumFacesLastOutput;

    //Geo-tagging
    EXIFData mEXIFData;

    //Image post-processing
    IPPMode mIPP;

    //jpeg Picture Quality
    unsigned int mPictureQuality;

    //thumbnail resolution
    unsigned int mThumbWidth, mThumbHeight;

    //thumbnail quality
    unsigned int mThumbQuality;

    //variables holding the estimated framerate
    float mFPS, mLastFPS;

    //automatically disable AF after a given amount of frames
    unsigned int mFocusThreshold;

    //This is needed for the CTS tests. They falsely assume, that during
    //smooth zoom the current zoom stage will not change within the
    //zoom callback scope, which in a real world situation is not always the
    //case. This variable will "simulate" the expected behavior
    unsigned int mZoomParameterIdx;

    //current zoom
    Mutex mZoomLock;
    unsigned int mCurrentZoomIdx, mTargetZoomIdx, mPreviousZoomIndx;
    bool mZoomUpdating, mZoomUpdate;
    int mZoomInc;
    bool mReturnZoomStatus;
    static const int32_t ZOOM_STEPS [];

     //local copy
    OMX_VERSIONTYPE mLocalVersionParam;

    unsigned int mPending3Asettings;
    Mutex m3ASettingsUpdateLock;
    Gen3A_settings mParameters3A;
    const char *mPictureFormatFromClient;

    OMX_TI_CONFIG_3A_FACE_PRIORITY mFacePriority;
    OMX_TI_CONFIG_3A_REGION_PRIORITY mRegionPriority;

    CameraParameters mParams;
    CameraProperties::Properties* mCapabilities;
    unsigned int mPictureRotation;
    bool mWaitingForSnapshot;
    int mSnapshotCount;
    bool mCaptureConfigured;
    unsigned int mPendingCaptureSettings;
    OMX_TI_ANCILLARYDATATYPE* mCaptureAncillaryData;
    OMX_TI_WHITEBALANCERESULTTYPE* mWhiteBalanceData;

    //Temporal bracketing management data
    mutable Mutex mBracketingLock;
    bool *mBracketingBuffersQueued;
    int mBracketingBuffersQueuedCount;
    int mLastBracetingBufferIdx;
    bool mBracketingEnabled;
    int mBracketingRange;

    bool mIternalRecordingHint;

    CameraParameters mParameters;
    bool mOmxInitialized;
    OMXCameraAdapterComponentContext mCameraAdapterParameters;
    bool mFirstTimeInit;

    ///Semaphores used internally
    Semaphore mInitSem;
    Semaphore mFlushSem;
    Semaphore mUsePreviewDataSem;
    Semaphore mUsePreviewSem;
    Semaphore mUseCaptureSem;
    Semaphore mStartPreviewSem;
    Semaphore mStopPreviewSem;
    Semaphore mStartCaptureSem;
    Semaphore mStopCaptureSem;
    Semaphore mSwitchToLoadedSem;
    Semaphore mSwitchToExecSem;

    mutable Mutex mStateSwitchLock;

    Vector<struct TIUTILS::Message *> mEventSignalQ;
    Mutex mEventLock;

    OMX_STATETYPE mComponentState;

    bool mVnfEnabled;
    bool mVstabEnabled;

    int mSensorOrientation;
    int mDeviceOrientation;
    bool mSensorOverclock;

    //Indicates if we should leave
    //OMX_Executing state during
    //stop-/startPreview
    bool mOMXStateSwitch;

    int mFrameCount;
    int mLastFrameCount;
    unsigned int mIter;
    nsecs_t mLastFPSTime;
    Mutex mFrameCountMutex;
    Condition mFirstFrameCondition;

    Mutex mDoAFMutex;
    Condition mDoAFCond;

    size_t mSensorIndex;
    CodingMode mCodingMode;

    // Time source delta of ducati & system time
    OMX_TICKS mTimeSourceDelta;
    bool onlyOnce;

    Semaphore mCaptureSem;
    bool mCaptureSignalled;

    OMX_BOOL mUserSetExpLock;
    OMX_BOOL mUserSetWbLock;

};
}; //// namespace
#endif //OMX_CAMERA_ADAPTER_H

