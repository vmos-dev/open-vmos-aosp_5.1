/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2010, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed toggle an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*!
 * \file      ExynosCamera.h
 * \brief     hearder file for CAMERA HAL MODULE
 * \author    thun.hwang(thun.hwang@samsung.com)
 * \date      2010/06/03
 *
 * <b>Revision History: </b>
 * - 2011/12/31 : thun.hwang(thun.hwang@samsung.com) \n
 *   Initial version
 *
 * - 2012/01/18 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Adjust Doxygen Document
 *
 * - 2012/02/01 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Adjust libv4l2
 *   Adjust struct ExynosCameraInfo
 *   External ISP feature
 *
 * - 2012/03/14 : sangwoo.park(sw5771.park@samsung.com) \n
 *   Change file, class name to ExynosXXX.
 */

#ifndef EXYNOS_CAMERA_H__
#define EXYNOS_CAMERA_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/videodev2.h>
#include <videodev2_exynos_camera.h>
#include <linux/vt.h>

#include <utils/RefBase.h>
#include <utils/String8.h>
#include "cutils/properties.h"

#include "exynos_format.h"
#include "ExynosBuffer.h"
#include "ExynosRect.h"
#include "ExynosJpegEncoderForCamera.h"
#include "ExynosExif.h"
#include "exynos_v4l2.h"

#define ALIGN(x,  a)       (((x) + (a) - 1) & ~((a) - 1))
namespace android {

#define GAIA_FW_BETA                        1
/* FIXME: This is for test. We remove this after test */
#define USE_DIGITAL_ZOOM

//! struct for Camera sensor information
/*!
 * \ingroup Exynos
 */
struct ExynosCameraInfo
{
public:
    // Google Official API : Camera.Parameters
    // http://developer.android.com/reference/android/hardware/Camera.Parameters.html
    int  previewW;
    int  previewH;
    int  previewColorFormat;
    int  videoW;
    int  videoH;
    int  videoColorFormat;
    int  pictureW;
    int  pictureH;
    int  pictureColorFormat;
    int  thumbnailW;
    int  thumbnailH;

    int  antiBandingList;
    int  antiBanding;

    int  effectList;
    int  effect;

    int  flashModeList;
    int  flashMode;

    int  focusModeList;
    int  focusMode;

    int  sceneModeList;
    int  sceneMode;

    int  whiteBalanceList;
    int  whiteBalance;
    bool autoWhiteBalanceLockSupported;
    bool autoWhiteBalanceLock;

    int  rotation;
    int  minExposure;
    int  maxExposure;
    int  exposure;

    bool autoExposureLockSupported;
    bool autoExposureLock;

    int  fps;
    int  focalLengthNum;
    int  focalLengthDen;
    bool supportVideoStabilization;
    bool applyVideoStabilization;
    bool videoStabilization;
    int  maxNumMeteringAreas;
    int  maxNumDetectedFaces;
    int  maxNumFocusAreas;
    int  maxZoom;
    bool hwZoomSupported;
    int  zoom;

    long gpsLatitude;
    long gpsLongitude;
    long gpsAltitude;
    long gpsTimestamp;

    // Additional API.
    int  angle;
    bool antiShake;
    bool beautyShot;
    int  brightness;
    int  contrast;
    bool gamma;
    bool odc;
    int  hue;
    int  iso;
    int  metering;
    bool objectTracking;
    bool objectTrackingStart;

    int  saturation;
    int  sharpness;
    int  shotMode;
    bool slowAE;
    bool smartAuto;
    bool touchAfStart;
    bool wdr;
    bool tdnr;

public:
    ExynosCameraInfo();
};

struct ExynosCameraInfoM5M0 : public ExynosCameraInfo
{
public:
    ExynosCameraInfoM5M0();
};

struct ExynosCameraInfoS5K6A3 : public ExynosCameraInfo
{
public:
    ExynosCameraInfoS5K6A3();
};

struct ExynosCameraInfoS5K4E5 : public ExynosCameraInfo
{
public:
    ExynosCameraInfoS5K4E5();
};

struct ExynosCameraInfoS5K3H7 : public ExynosCameraInfo
{
public:
    ExynosCameraInfoS5K3H7();
};

//! ExynosCamera
/*!
 * \ingroup Exynos
 */
class ExynosCamera : public virtual RefBase {

///////////////////////////////////////////////////
// Google Official API : Camera.Parameters
// http://developer.android.com/reference/android/hardware/Camera.Parameters.html
///////////////////////////////////////////////////
public:
    //! Camera ID
    enum CAMERA_ID {
        CAMERA_ID_BACK  = 0,   //!<
        CAMERA_ID_FRONT = 1,   //!<
    };

    //! Anti banding
    enum {
        ANTIBANDING_AUTO = (1 << 0), //!< \n
        ANTIBANDING_50HZ = (1 << 1), //!< \n
        ANTIBANDING_60HZ = (1 << 2), //!< \n
        ANTIBANDING_OFF  = (1 << 3), //!< \n
    };

    //! Effect
    enum {
        EFFECT_NONE       = (1 << 0), //!< \n
        EFFECT_MONO       = (1 << 1), //!< \n
        EFFECT_NEGATIVE   = (1 << 2), //!< \n
        EFFECT_SOLARIZE   = (1 << 3), //!< \n
        EFFECT_SEPIA      = (1 << 4), //!< \n
        EFFECT_POSTERIZE  = (1 << 5), //!< \n
        EFFECT_WHITEBOARD = (1 << 6), //!< \n
        EFFECT_BLACKBOARD = (1 << 7), //!< \n
        EFFECT_AQUA       = (1 << 8), //!< \n
    };

    //! Flash mode
    enum {
        FLASH_MODE_OFF     = (1 << 0), //!< \n
        FLASH_MODE_AUTO    = (1 << 1), //!< \n
        FLASH_MODE_ON      = (1 << 2), //!< \n
        FLASH_MODE_RED_EYE = (1 << 3), //!< \n
        FLASH_MODE_TORCH   = (1 << 4), //!< \n
    };

    //! Focus mode
    enum {
        FOCUS_MODE_AUTO               = (1 << 0), //!< \n
        FOCUS_MODE_INFINITY           = (1 << 1), //!< \n
        FOCUS_MODE_MACRO              = (1 << 2), //!< \n
        FOCUS_MODE_FIXED              = (1 << 3), //!< \n
        FOCUS_MODE_EDOF               = (1 << 4), //!< \n
        FOCUS_MODE_CONTINUOUS_VIDEO   = (1 << 5), //!< \n
        FOCUS_MODE_CONTINUOUS_PICTURE = (1 << 6), //!< \n
        FOCUS_MODE_TOUCH              = (1 << 7), //!< \n
    };

    //! Scene mode
    enum {
        SCENE_MODE_AUTO           = (1 << 0), //!< \n
        SCENE_MODE_ACTION         = (1 << 1), //!< \n
        SCENE_MODE_PORTRAIT       = (1 << 2), //!< \n
        SCENE_MODE_LANDSCAPE      = (1 << 3), //!< \n
        SCENE_MODE_NIGHT          = (1 << 4), //!< \n
        SCENE_MODE_NIGHT_PORTRAIT = (1 << 5), //!< \n
        SCENE_MODE_THEATRE        = (1 << 6), //!< \n
        SCENE_MODE_BEACH          = (1 << 7), //!< \n
        SCENE_MODE_SNOW           = (1 << 8), //!< \n
        SCENE_MODE_SUNSET         = (1 << 9), //!< \n
        SCENE_MODE_STEADYPHOTO    = (1 << 10), //!< \n
        SCENE_MODE_FIREWORKS      = (1 << 11), //!< \n
        SCENE_MODE_SPORTS         = (1 << 12), //!< \n
        SCENE_MODE_PARTY          = (1 << 13), //!< \n
        SCENE_MODE_CANDLELIGHT    = (1 << 14), //!< \n
    };

    //! White balance
    enum {
        WHITE_BALANCE_AUTO             = (1 << 0), //!< \n
        WHITE_BALANCE_INCANDESCENT     = (1 << 1), //!< \n
        WHITE_BALANCE_FLUORESCENT      = (1 << 2), //!< \n
        WHITE_BALANCE_WARM_FLUORESCENT = (1 << 3), //!< \n
        WHITE_BALANCE_DAYLIGHT         = (1 << 4), //!< \n
        WHITE_BALANCE_CLOUDY_DAYLIGHT  = (1 << 5), //!< \n
        WHITE_BALANCE_TWILIGHT         = (1 << 6), //!< \n
        WHITE_BALANCE_SHADE            = (1 << 7), //!< \n
    };

    //! Jpeg Qualtiy
    enum JPEG_QUALITY {
        JPEG_QUALITY_MIN        = 0,    //!<
        JPEG_QUALITY_ECONOMY    = 70,   //!<
        JPEG_QUALITY_NORMAL     = 80,   //!<
        JPEG_QUALITY_SUPERFINE  = 90,   //!<
        JPEG_QUALITY_MAX        = 100,  //!<
    };

private:
    //! Constructor
    ExynosCamera();
    //! Destructor
    virtual ~ExynosCamera();

public:
    //! Gets the Camera instance
    static ExynosCamera* createInstance(void)
    {
        static ExynosCamera singleton;
        return &singleton;
    }

    //! Create the instance
    bool            create(int cameraId);
    //! Destroy the instance
    bool            destroy(void);
    //! Check if the instance was created
    bool            flagCreate(void);

    //! Gets current camera_id
    int             getCameraId(void);
    //! Gets camera sensor name
    char           *getCameraName(void);

    //! Gets file descriptor by gotten open() for preview
    int             getPreviewFd(void);
    //! Gets file descriptor by gotten open() for recording
    int             getVideoFd(void);
    //! Gets file descriptor by gotten open() for snapshot
    int             getPictureFd(void);

    //! Starts capturing and drawing preview frames to the screen.
    bool            startPreview(void);
    //! Stop preview
    bool            stopPreview(void);
    //! Check preview start
    bool            flagStartPreview(void);
    //! Gets preview's max buffer
    int             getPreviewMaxBuf(void);
    //! Sets preview's buffer
    bool            setPreviewBuf(ExynosBuffer *buf);
    //! Gets preview's buffer
    bool            getPreviewBuf(ExynosBuffer *buf);
    //! Put(dq) preview's buffer
    bool            putPreviewBuf(ExynosBuffer *buf);

    //! Sets video's width, height
    bool            setVideoSize(int w, int h);
    //! Gets video's width, height
    bool            getVideoSize(int *w, int *h);

    //! Sets video's color format
    bool            setVideoFormat(int colorFormat);
    //! Gets video's color format
    int             getVideoFormat(void);

    //! Start video
    bool            startVideo(void);
    //! Stop video
    bool            stopVideo(void);
    //! Check video start
    bool            flagStartVideo(void);
    //! Gets video's buffer
    int             getVideoMaxBuf(void);
    //! Sets video's buffer
    bool            setVideoBuf(ExynosBuffer *buf);
    //! Gets video's buffer
    bool            getVideoBuf(ExynosBuffer *buf);
    //! Put(dq) video's buffer
    bool            putVideoBuf(ExynosBuffer *buf);

    //! Start snapshot
    bool            startPicture(void);
    //! Stop snapshot
    bool            stopPicture(void);
    //! Check snapshot start
    bool            flagStartPicture(void);
    //! Gets snapshot's buffer
    int             getPictureMaxBuf(void);
    //! Sets snapshot's buffer
    bool            setPictureBuf(ExynosBuffer *buf);
    //! Gets snapshot's buffer
    bool            getPictureBuf(ExynosBuffer *buf);
    //! Put(dq) snapshot's buffer
    bool            putPictureBuf(ExynosBuffer *buf);

    //! Encode JPEG from YUV
    bool            yuv2Jpeg(ExynosBuffer *yuvBuf, ExynosBuffer *jpegBuf, ExynosRect *rect);

    //! Starts camera auto-focus and registers a callback function to run when the camera is focused.
    bool            autoFocus(void);
    //! Cancel auto-focus operation
    bool            cancelAutoFocus(void);
    //! Gets auto-focus result whether success or not
    int             getFucusModeResult(void);

    //! Starts the face detection.
    bool            startFaceDetection(void);
    //! Stop face detection
    bool            stopFaceDetection(void);
    //! Gets the face detection started
    bool            flagStartFaceDetection(void);
    //! Lock or unlock face detection operation
    bool            setFaceDetectLock(bool toggle);

    //! Zooms to the requested value smoothly.
    bool            startSmoothZoom(int value);
    //! Stop the face detection.
    bool            stopSmoothZoom(void);

    //! Gets the current antibanding setting.
    int             getAntibanding(void);

    //! Gets the state of the auto-exposure lock.
    bool            getAutoExposureLock(void);

    //! Gets the state of the auto-white balance lock.
    bool            getAutoWhiteBalanceLock(void);

    //! Gets the current color effect setting.
    int             getColorEffect(void);

    //! Gets the detected faces areas.
    int             getDetectedFacesAreas(int num, int *id, int *score, ExynosRect *face, ExynosRect *leftEye, ExynosRect *rightEye, ExynosRect *mouth);

    //! Gets the detected faces areas. (Using ExynosRect2)
    int             getDetectedFacesAreas(int num, int *id, int *score, ExynosRect2 *face, ExynosRect2 *leftEye, ExynosRect2 *rightEye, ExynosRect2 *mouth);

    //! Gets the current exposure compensation index.
    int             getExposureCompensation(void);

    //! Gets the exposure compensation step.
    float           getExposureCompensationStep(void);

    //! Gets the current flash mode setting.
    int             getFlashMode(void);

    //! Gets the focal length (in millimeter) of the camera.
    bool            getFocalLength(int *num, int *den);

    //! Gets the current focus areas.
    int             getFocusAreas(ExynosRect *rects);

    //! Gets the distances from the camera to where an object appears to be in focus.
    int             getFocusDistances(float *output);

    //! Gets the current focus mode setting.
    int             getFocusMode(void);

    //! Gets the horizontal angle of view in degrees.
    float           getHorizontalViewAngle(void);

    //int             getInt(String key);

    //! Returns the quality setting for the JPEG picture.
    int             getJpegQuality(void);

    //! Returns the quality setting for the EXIF thumbnail in Jpeg picture.
    int             getJpegThumbnailQuality(void);

    //! Returns the dimensions for EXIF thumbnail in Jpeg picture.
    bool            getJpegThumbnailSize(int *w, int *h);

    //! Gets the maximum exposure compensation index.
    int             getMaxExposureCompensation(void);

    //! Gets the maximum number of detected faces supported.
    int             getMaxNumDetectedFaces(void);

    //! Gets the maximum number of focus areas supported.
    int             getMaxNumFocusAreas(void);

    //! Gets the maximum number of metering areas supported.
    int             getMaxNumMeteringAreas(void);

    //! Gets the maximum zoom value allowed for snapshot.
    int             getMaxZoom(void);

    //! Gets the current metering areas.
    int             getMeteringAreas(ExynosRect *rects);

    //! Gets the minimum exposure compensation index.
    int             getMinExposureCompensation(void);

    //! Returns the image format for pictures.
    int             getPictureFormat(void);

    //! Returns the dimension setting for pictures.
    bool            getPictureSize(int *w, int *h);

    //Camera.Size     getPreferredPreviewSizeForVideo();

    //! Returns the image format for preview frames got from Camera.PreviewCallback.
    int             getPreviewFormat(void);

    //! Returns the current minimum and maximum preview fps.
    bool            getPreviewFpsRange(int *min, int *max);

    //! This method is deprecated. replaced by getPreviewFpsRange(int[])
    int             getPreviewFrameRate(void);

    //! Returns the dimensions setting for preview pictures.
    bool            getPreviewSize(int *w, int *h);

    //! Gets scene mode
    int             getSceneMode(void);

    //! Gets the supported antibanding values.
    int             getSupportedAntibanding(void);

    //! Gets the supported color effects.
    int             getSupportedColorEffects(void);

    //! Check whether the target support Flash
    int             getSupportedFlashModes(void);

    //! Gets the supported focus modes.
    int             getSupportedFocusModes(void);

    //! Gets the supported jpeg thumbnail sizes.
    bool            getSupportedJpegThumbnailSizes(int *w, int *h);

    // List<Integer>  getSupportedPictureFormats()

    //! Gets the supported picture sizes.
    bool            getSupportedPictureSizes(int *w, int *h);

    //List<Integer>   getSupportedPreviewFormats()

    //List<int[]>     getSupportedPreviewFpsRange()

    //List<Integer>   getSupportedPreviewFrameRates()

    //! Gets the supported preview sizes.
    bool            getSupportedPreviewSizes(int *w, int *h);

    //! Gets the supported scene modes.
    int             getSupportedSceneModes(void);

    //! Gets the supported video frame sizes that can be used by MediaRecorder.
    bool            getSupportedVideoSizes(int *w, int *h);

    //! Gets the supported white balance.
    int             getSupportedWhiteBalance(void);

    //! Gets the vertical angle of view in degrees.
    float           getVerticalViewAngle(void);

    //! Gets the current state of video stabilization.
    bool            getVideoStabilization(void);

    //! Gets the current white balance setting.
    int             getWhiteBalance(void);

    //! Gets current zoom value.
    int             getZoom(void);

    //List<Integer>   getZoomRatios()
    //! Gets max zoom ratio
    int             getMaxZoomRatio(void);

    //! Returns true if auto-exposure locking is supported.
    bool            isAutoExposureLockSupported(void);

    //! Returns true if auto-white balance locking is supported.
    bool            isAutoWhiteBalanceLockSupported(void);

    //! Returns true if smooth zoom is supported.
    bool            isSmoothZoomSupported(void);

    //! Returns true if video snapshot is supported.
    bool            isVideoSnapshotSupported(void);

    //! Returns true if video stabilization is supported.
    bool            isVideoStabilizationSupported(void);

    //! Returns true if zoom is supported.
    bool            isZoomSupported(void);

    //void            remove(String key)

    //void            removeGpsData()

    //void            set(String key, String value)

    //void            set(String key, int value)

    //! Sets the antibanding.
    bool            setAntibanding(int value);

    //! Sets the auto-exposure lock state.
    bool            setAutoExposureLock(bool toggle);

    //! Sets the auto-white balance lock state.
    bool            setAutoWhiteBalanceLock(bool toggle);

    //! Sets the current color effect setting.
    bool            setColorEffect(int value);

    //! Sets the exposure compensation index.
    bool            setExposureCompensation(int value);

    //! Sets the flash mode.
    bool            setFlashMode(int value);

    //! Sets focus z.
    bool            setFocusAreas(int num, ExynosRect* rects, int *weights);

    //! Sets focus areas. (Using ExynosRect2)
    bool            setFocusAreas(int num, ExynosRect2* rect2s, int *weights);

    //! Sets the focus mode.
    bool            setFocusMode(int value);

    //! Sets GPS altitude.
    bool            setGpsAltitude(const char *gpsAltitude);

    //! Sets GPS latitude coordinate.
    bool            setGpsLatitude(const char *gpsLatitude);

    //! Sets GPS longitude coordinate.
    bool            setGpsLongitude(const char *gpsLongitude);

    //! Sets GPS processing method.
    bool            setGpsProcessingMethod(const char *gpsProcessingMethod);

    //! Sets GPS timestamp.
    bool            setGpsTimeStamp(const char *gpsTimestamp);

    //! Sets Jpeg quality of captured picture.
    bool            setJpegQuality(int quality);

    //! Sets the quality of the EXIF thumbnail in Jpeg picture.
    bool            setJpegThumbnailQuality(int quality);

    //! Sets the dimensions for EXIF thumbnail in Jpeg picture.
    bool            setJpegThumbnailSize(int w, int h);

    //! Sets metering areas.
    bool            setMeteringAreas(int num, ExynosRect  *rects, int *weights);

    //! Sets metering areas.(Using ExynosRect2)
    bool            setMeteringAreas(int num, ExynosRect2 *rect2s, int *weights);

    //! Cancel metering areas.
    bool            cancelMeteringAreas();

    //! Sets the image format for pictures.
    bool            setPictureFormat(int colorFormat);

    //! Sets the dimensions for pictures.
    bool            setPictureSize(int w, int h);

    //! Sets the image format for preview pictures.
    bool            setPreviewFormat(int colorFormat);

    //void            setPreviewFpsRange(int min, int max)

    // ! This method is deprecated. replaced by setPreviewFpsRange(int, int)
    bool            setPreviewFrameRate(int fps);

    //! Sets the dimensions for preview pictures.
    bool            setPreviewSize(int w, int h);

    //! Sets recording mode hint.
    bool            setRecordingHint(bool hint);

    //! Sets the rotation angle in degrees relative to the orientation of the camera.
    bool            setRotation(int rotation);

    //! Gets the rotation angle in degrees relative to the orientation of the camera.
    int             getRotation(void);

    //! Sets the scene mode.
    bool            setSceneMode(int value);

    //! Enables and disables video stabilization.
    bool            setVideoStabilization(bool toggle);

    //! Sets the white balance.
    bool            setWhiteBalance(int value);

    //! Sets current zoom value.
    bool            setZoom(int value);

    //void            unflatten(String flattened)

private:
    enum MODE
    {
        PREVIEW_MODE  = 0,
        VIDEO_MODE,
        PICTURE_MODE,
    };

    struct devInfo {
        int    fd;
        pollfd events;
        bool   flagStart;
    };

    bool            m_flagCreate;

    int             m_cameraId;

    ExynosCameraInfo  *m_defaultCameraInfo;
    ExynosCameraInfo  *m_curCameraInfo;

    int             m_jpegQuality;
    int             m_jpegThumbnailQuality;

    int             m_currentZoom;
    bool            m_recordingHint;

    // v4l2 sub-dev file description
    devInfo         m_sensorDev;
    devInfo         m_mipiDev;
    devInfo         m_fliteDev;
    devInfo         m_gscPreviewDev;
    devInfo         m_gscVideoDev;
    devInfo         m_gscPictureDev;

#ifdef USE_DIGITAL_ZOOM
    devInfo         m_gscBayerDev;
    devInfo        *m_bayerDev;
#endif

    devInfo        *m_previewDev;
    devInfo        *m_videoDev;
    devInfo        *m_pictureDev;

    bool            m_tryPreviewStop;
    bool            m_tryVideoStop;
    bool            m_tryPictureStop;

    bool            m_flagStartFaceDetection;
    bool            m_flagAutoFocusRunning;

    char            m_cameraName[32];
    bool            m_internalISP;
    bool            m_touchAFMode;
    bool            m_isTouchMetering;

    bool            m_focusIdle;

    // media controller variable
    struct media_device *m_media;
    struct media_entity *m_sensorEntity;
    struct media_entity *m_mipiEntity;
    struct media_entity *m_fliteSdEntity;
    struct media_entity *m_fliteVdEntity;
    struct media_entity *m_gscSdEntity;
    struct media_entity *m_gscVdEntity;
    struct media_entity *m_ispSensorEntity;
    struct media_entity *m_ispFrontEntity;
    struct media_entity *m_ispBackEntity;
    struct media_entity *m_ispBayerEntity;
    struct media_entity *m_ispScalercEntity;
    struct media_entity *m_ispScalerpEntity;
    struct media_entity *m_isp3dnrEntity;

    bool   m_validPreviewBuf[VIDEO_MAX_FRAME];
    bool   m_validVideoBuf[VIDEO_MAX_FRAME];
    bool   m_validPictureBuf[VIDEO_MAX_FRAME];

    struct ExynosBuffer m_previewBuf[VIDEO_MAX_FRAME];
    struct ExynosBuffer m_videoBuf[VIDEO_MAX_FRAME];
    struct ExynosBuffer m_pictureBuf[VIDEO_MAX_FRAME];

    exif_attribute_t mExifInfo;

private:
    bool            m_setWidthHeight(int mode,
                                     int fd,
                                     struct pollfd *event,
                                     int w,
                                     int h,
                                     int colorFormat,
                                     struct ExynosBuffer *buf,
                                     bool *validBuf);
    bool            m_setZoom(int fd, int zoom, int w, int h);
    bool            m_setCrop(int fd, int w, int h, int zoom);
    bool            m_getCropRect(unsigned int  src_w,  unsigned int   src_h,
                              unsigned int  dst_w,  unsigned int   dst_h,
                              unsigned int *crop_x, unsigned int *crop_y,
                              unsigned int *crop_w, unsigned int *crop_h,
                              int           zoom);

    void            m_setExifFixedAttribute(void);
    void            m_setExifChangedAttribute(exif_attribute_t *exifInfo, ExynosRect *rect);
    void            m_secRect2SecRect2(ExynosRect *rect, ExynosRect2 *rect2);
    void            m_secRect22SecRect(ExynosRect2 *rect2, ExynosRect *rect);
    void            m_printFormat(int colorFormat, const char *arg);

///////////////////////////////////////////////////
// Additional API.
///////////////////////////////////////////////////
public:
    //! Focus mode
    enum {
        FOCUS_MODE_CONTINUOUS_PICTURE_MACRO = (1 << 8), //!< \n
    };

    //! Metering
    enum {
        METERING_MODE_AVERAGE = (1 << 0), //!< \n
        METERING_MODE_CENTER  = (1 << 1), //!< \n
        METERING_MODE_MATRIX  = (1 << 2), //!< \n
        METERING_MODE_SPOT    = (1 << 3), //!< \n
    };

    //! Contrast
    enum {
        CONTRAST_AUTO    = (1 << 0), //!< \n
        CONTRAST_MINUS_2 = (1 << 1), //!< \n
        CONTRAST_MINUS_1 = (1 << 2), //!< \n
        CONTRAST_DEFAULT = (1 << 3), //!< \n
        CONTRAST_PLUS_1  = (1 << 4), //!< \n
        CONTRAST_PLUS_2  = (1 << 5), //!< \n
    };
    //! Camera Shot mode
    enum SHOT_MODE {
        SHOT_MODE_SINGLE        = 0, //!<
        SHOT_MODE_CONTINUOUS    = 1, //!<
        SHOT_MODE_PANORAMA      = 2, //!<
        SHOT_MODE_SMILE         = 3, //!<
        SHOT_MODE_SELF          = 6, //!<
    };

    //! Sets camera angle
    bool            setAngle(int angle);

    //! Gets camera angle
    int             getAngle(void);

    //! Sets metering areas.
    bool            setMeteringMode(int value);
    //! Gets metering
    int             getMeteringMode(void);

    //! Sets Top-down mirror
    bool            setTopDownMirror(void);
    //! Sets Left-right mirror
    bool            setLRMirror(void);

    //! Sets brightness
    bool            setBrightness(int brightness);
    //! Gets brightness
    int             getBrightness(void);

    //! Sets ISO
    bool            setISO(int iso);
    //! Gets ISO
    int             getISO(void);

    //! Sets Contrast
    bool            setContrast(int value);
    //! Gets Contrast
    int             getContrast(void);

    //! Sets Saturation
    bool            setSaturation(int saturation);
    //! Gets Saturation
    int             getSaturation(void);

    //! Sets Sharpness
    bool            setSharpness(int sharpness);
    //! Gets Sharpness
    int             getSharpness(void);

    // ! Sets Hue
    bool            setHue(int hue);
    // ! Gets Hue
    int             getHue(void);

    //! Sets WDR
    bool            setWDR(bool toggle);
    //! Gets WDR
    bool            getWDR(void);

    //! Sets anti shake
    bool            setAntiShake(bool toggle);
    //! Gets anti shake
    bool            getAntiShake(void);

    //! Sets object tracking
    bool            setObjectTracking(bool toggle);
    //! Gets object tracking
    bool            getObjectTracking(void);
    //! Start or stop object tracking operation
    bool            setObjectTrackingStart(bool toggle);
    //! Gets status of object tracking operation
    int             getObjectTrackingStatus(void);
    //! Sets x, y position for object tracking operation
    bool            setObjectPosition(int x, int y);

    //! Sets smart auto
    bool            setSmartAuto(bool toggle);
    //! Gets smart auto
    bool            getSmartAuto(void);
    //! Gets the status of smart auto operation
    int             getSmartAutoStatus(void);

    //! Sets beauty shot
    bool            setBeautyShot(bool toggle);
    //! Gets beauty shot
    bool            getBeautyShot(void);

    //! Start or stop the touch auto focus operation
    bool            setTouchAFStart(bool toggle);

    //! Sets gamma
    bool            setGamma(bool toggle);
    //! Gets gamma
    bool            getGamma(void);

    //! Sets ODC
    bool            setODC(bool toggle);
    //! Gets ODC
    bool            getODC(void);

    //! Sets Slow AE
    bool            setSlowAE(bool toggle);
    //! Gets Slow AE
    bool            getSlowAE(void);

    //! Sets Shot mode
    bool            setShotMode(int shotMode);
    //! Gets Shot mode
    int             getShotMode(void);

    //! Sets 3DNR
    bool            set3DNR(bool toggle);
    //! Gets 3DNR
    bool            get3DNR(void);
};

}; // namespace android

#endif // EXYNOS_CAMERA_H__
