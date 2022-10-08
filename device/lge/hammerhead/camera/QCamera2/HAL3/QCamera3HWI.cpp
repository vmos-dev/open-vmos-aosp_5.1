/* Copyright (c) 2012-2014, The Linux Foundataion. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#define LOG_TAG "QCamera3HWI"
//#define LOG_NDEBUG 0

#define __STDC_LIMIT_MACROS
#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <camera/CameraMetadata.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <ui/Fence.h>
#include <gralloc_priv.h>
#include "QCamera3HWI.h"
#include "QCamera3Mem.h"
#include "QCamera3Channel.h"
#include "QCamera3PostProc.h"
#include "QCamera3VendorTags.h"

using namespace android;

namespace qcamera {

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define DATA_PTR(MEM_OBJ,INDEX) MEM_OBJ->getPtr( INDEX )

#define EMPTY_PIPELINE_DELAY 2
#define CAM_MAX_SYNC_LATENCY 4

cam_capability_t *gCamCapability[MM_CAMERA_MAX_NUM_SENSORS];
const camera_metadata_t *gStaticMetadata[MM_CAMERA_MAX_NUM_SENSORS];

pthread_mutex_t QCamera3HardwareInterface::mCameraSessionLock =
    PTHREAD_MUTEX_INITIALIZER;
unsigned int QCamera3HardwareInterface::mCameraSessionActive = 0;

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::EFFECT_MODES_MAP[] = {
    { ANDROID_CONTROL_EFFECT_MODE_OFF,       CAM_EFFECT_MODE_OFF },
    { ANDROID_CONTROL_EFFECT_MODE_MONO,       CAM_EFFECT_MODE_MONO },
    { ANDROID_CONTROL_EFFECT_MODE_NEGATIVE,   CAM_EFFECT_MODE_NEGATIVE },
    { ANDROID_CONTROL_EFFECT_MODE_SOLARIZE,   CAM_EFFECT_MODE_SOLARIZE },
    { ANDROID_CONTROL_EFFECT_MODE_SEPIA,      CAM_EFFECT_MODE_SEPIA },
    { ANDROID_CONTROL_EFFECT_MODE_POSTERIZE,  CAM_EFFECT_MODE_POSTERIZE },
    { ANDROID_CONTROL_EFFECT_MODE_WHITEBOARD, CAM_EFFECT_MODE_WHITEBOARD },
    { ANDROID_CONTROL_EFFECT_MODE_BLACKBOARD, CAM_EFFECT_MODE_BLACKBOARD },
    { ANDROID_CONTROL_EFFECT_MODE_AQUA,       CAM_EFFECT_MODE_AQUA }
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::WHITE_BALANCE_MODES_MAP[] = {
    { ANDROID_CONTROL_AWB_MODE_OFF,             CAM_WB_MODE_OFF },
    { ANDROID_CONTROL_AWB_MODE_AUTO,            CAM_WB_MODE_AUTO },
    { ANDROID_CONTROL_AWB_MODE_INCANDESCENT,    CAM_WB_MODE_INCANDESCENT },
    { ANDROID_CONTROL_AWB_MODE_FLUORESCENT,     CAM_WB_MODE_FLUORESCENT },
    { ANDROID_CONTROL_AWB_MODE_WARM_FLUORESCENT,CAM_WB_MODE_WARM_FLUORESCENT},
    { ANDROID_CONTROL_AWB_MODE_DAYLIGHT,        CAM_WB_MODE_DAYLIGHT },
    { ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT, CAM_WB_MODE_CLOUDY_DAYLIGHT },
    { ANDROID_CONTROL_AWB_MODE_TWILIGHT,        CAM_WB_MODE_TWILIGHT },
    { ANDROID_CONTROL_AWB_MODE_SHADE,           CAM_WB_MODE_SHADE }
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::SCENE_MODES_MAP[] = {
    { ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY,  CAM_SCENE_MODE_OFF },
    { ANDROID_CONTROL_SCENE_MODE_ACTION,         CAM_SCENE_MODE_ACTION },
    { ANDROID_CONTROL_SCENE_MODE_PORTRAIT,       CAM_SCENE_MODE_PORTRAIT },
    { ANDROID_CONTROL_SCENE_MODE_LANDSCAPE,      CAM_SCENE_MODE_LANDSCAPE },
    { ANDROID_CONTROL_SCENE_MODE_NIGHT,          CAM_SCENE_MODE_NIGHT },
    { ANDROID_CONTROL_SCENE_MODE_NIGHT_PORTRAIT, CAM_SCENE_MODE_NIGHT_PORTRAIT },
    { ANDROID_CONTROL_SCENE_MODE_THEATRE,        CAM_SCENE_MODE_THEATRE },
    { ANDROID_CONTROL_SCENE_MODE_BEACH,          CAM_SCENE_MODE_BEACH },
    { ANDROID_CONTROL_SCENE_MODE_SNOW,           CAM_SCENE_MODE_SNOW },
    { ANDROID_CONTROL_SCENE_MODE_SUNSET,         CAM_SCENE_MODE_SUNSET },
    { ANDROID_CONTROL_SCENE_MODE_STEADYPHOTO,    CAM_SCENE_MODE_ANTISHAKE },
    { ANDROID_CONTROL_SCENE_MODE_FIREWORKS ,     CAM_SCENE_MODE_FIREWORKS },
    { ANDROID_CONTROL_SCENE_MODE_SPORTS ,        CAM_SCENE_MODE_SPORTS },
    { ANDROID_CONTROL_SCENE_MODE_PARTY,          CAM_SCENE_MODE_PARTY },
    { ANDROID_CONTROL_SCENE_MODE_CANDLELIGHT,    CAM_SCENE_MODE_CANDLELIGHT },
    { ANDROID_CONTROL_SCENE_MODE_BARCODE,        CAM_SCENE_MODE_BARCODE}
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::FOCUS_MODES_MAP[] = {
    { ANDROID_CONTROL_AF_MODE_OFF,                CAM_FOCUS_MODE_OFF },
    { ANDROID_CONTROL_AF_MODE_OFF,                CAM_FOCUS_MODE_FIXED },
    { ANDROID_CONTROL_AF_MODE_AUTO,               CAM_FOCUS_MODE_AUTO },
    { ANDROID_CONTROL_AF_MODE_MACRO,              CAM_FOCUS_MODE_MACRO },
    { ANDROID_CONTROL_AF_MODE_EDOF,               CAM_FOCUS_MODE_EDOF },
    { ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE, CAM_FOCUS_MODE_CONTINOUS_PICTURE },
    { ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO,   CAM_FOCUS_MODE_CONTINOUS_VIDEO }
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::ANTIBANDING_MODES_MAP[] = {
    { ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF,  CAM_ANTIBANDING_MODE_OFF },
    { ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ, CAM_ANTIBANDING_MODE_50HZ },
    { ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ, CAM_ANTIBANDING_MODE_60HZ },
    { ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO, CAM_ANTIBANDING_MODE_AUTO }
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::AE_FLASH_MODE_MAP[] = {
    { ANDROID_CONTROL_AE_MODE_OFF,                  CAM_FLASH_MODE_OFF },
    { ANDROID_CONTROL_AE_MODE_ON,                   CAM_FLASH_MODE_OFF },
    { ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH,        CAM_FLASH_MODE_AUTO},
    { ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH,      CAM_FLASH_MODE_ON  },
    { ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE, CAM_FLASH_MODE_AUTO}
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::FLASH_MODES_MAP[] = {
    { ANDROID_FLASH_MODE_OFF,    CAM_FLASH_MODE_OFF  },
    { ANDROID_FLASH_MODE_SINGLE, CAM_FLASH_MODE_SINGLE },
    { ANDROID_FLASH_MODE_TORCH,  CAM_FLASH_MODE_TORCH }
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::FACEDETECT_MODES_MAP[] = {
    { ANDROID_STATISTICS_FACE_DETECT_MODE_OFF,    CAM_FACE_DETECT_MODE_OFF     },
    { ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE, CAM_FACE_DETECT_MODE_SIMPLE  },
    { ANDROID_STATISTICS_FACE_DETECT_MODE_FULL,   CAM_FACE_DETECT_MODE_FULL    }
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::FOCUS_CALIBRATION_MAP[] = {
    { ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED,
      CAM_FOCUS_UNCALIBRATED },
    { ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_APPROXIMATE,
      CAM_FOCUS_APPROXIMATE },
    { ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED,
      CAM_FOCUS_CALIBRATED }
};

const int32_t available_thumbnail_sizes[] = {0, 0,
                                             176, 144,
                                             320, 240,
                                             432, 288,
                                             480, 288,
                                             512, 288,
                                             512, 384};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::TEST_PATTERN_MAP[] = {
    { ANDROID_SENSOR_TEST_PATTERN_MODE_OFF,          CAM_TEST_PATTERN_OFF   },
    { ANDROID_SENSOR_TEST_PATTERN_MODE_SOLID_COLOR,  CAM_TEST_PATTERN_SOLID_COLOR },
    { ANDROID_SENSOR_TEST_PATTERN_MODE_COLOR_BARS,   CAM_TEST_PATTERN_COLOR_BARS },
    { ANDROID_SENSOR_TEST_PATTERN_MODE_COLOR_BARS_FADE_TO_GRAY, CAM_TEST_PATTERN_COLOR_BARS_FADE_TO_GRAY },
    { ANDROID_SENSOR_TEST_PATTERN_MODE_PN9,          CAM_TEST_PATTERN_PN9 },
};

/* Since there is no mapping for all the options some Android enum are not listed.
 * Also, the order in this list is important because while mapping from HAL to Android it will
 * traverse from lower to higher index which means that for HAL values that are map to different
 * Android values, the traverse logic will select the first one found.
 */
const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::REFERENCE_ILLUMINANT_MAP[] = {
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_FLUORESCENT, CAM_AWB_WARM_FLO},
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT_FLUORESCENT, CAM_AWB_CUSTOM_DAYLIGHT },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_COOL_WHITE_FLUORESCENT, CAM_AWB_COLD_FLO },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A, CAM_AWB_A },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D55, CAM_AWB_NOON },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65, CAM_AWB_D65 },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D75, CAM_AWB_D75 },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D50, CAM_AWB_D50 },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_ISO_STUDIO_TUNGSTEN, CAM_AWB_CUSTOM_A},
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT, CAM_AWB_D50 },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_TUNGSTEN, CAM_AWB_A },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_FINE_WEATHER, CAM_AWB_D50 },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_CLOUDY_WEATHER, CAM_AWB_D65 },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_SHADE, CAM_AWB_D75 },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAY_WHITE_FLUORESCENT, CAM_AWB_CUSTOM_DAYLIGHT },
    { ANDROID_SENSOR_REFERENCE_ILLUMINANT1_WHITE_FLUORESCENT, CAM_AWB_COLD_FLO},
};

/* Custom tag definitions */

camera3_device_ops_t QCamera3HardwareInterface::mCameraOps = {
    initialize:                         QCamera3HardwareInterface::initialize,
    configure_streams:                  QCamera3HardwareInterface::configure_streams,
    register_stream_buffers:            NULL,
    construct_default_request_settings: QCamera3HardwareInterface::construct_default_request_settings,
    process_capture_request:            QCamera3HardwareInterface::process_capture_request,
    get_metadata_vendor_tag_ops:        NULL,
    dump:                               QCamera3HardwareInterface::dump,
    flush:                              QCamera3HardwareInterface::flush,
    reserved:                           {0},
};

int QCamera3HardwareInterface::kMaxInFlight = 5;

/*===========================================================================
 * FUNCTION   : QCamera3HardwareInterface
 *
 * DESCRIPTION: constructor of QCamera3HardwareInterface
 *
 * PARAMETERS :
 *   @cameraId  : camera ID
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3HardwareInterface::QCamera3HardwareInterface(int cameraId,
                        const camera_module_callbacks_t *callbacks)
    : mCameraId(cameraId),
      mCameraHandle(NULL),
      mCameraOpened(false),
      mCameraInitialized(false),
      mCallbackOps(NULL),
      mInputStream(NULL),
      mMetadataChannel(NULL),
      mPictureChannel(NULL),
      mRawChannel(NULL),
      mSupportChannel(NULL),
      mFirstRequest(false),
      mRepeatingRequest(false),
      mParamHeap(NULL),
      mParameters(NULL),
      mPrevParameters(NULL),
      mLoopBackResult(NULL),
      mFlush(false),
      mMinProcessedFrameDuration(0),
      mMinJpegFrameDuration(0),
      mMinRawFrameDuration(0),
      m_pPowerModule(NULL),
      mHdrHint(false),
      mMetaFrameCount(0),
      mCallbacks(callbacks)
{
    mCameraDevice.common.tag = HARDWARE_DEVICE_TAG;
    mCameraDevice.common.version = CAMERA_DEVICE_API_VERSION_3_2;
    mCameraDevice.common.close = close_camera_device;
    mCameraDevice.ops = &mCameraOps;
    mCameraDevice.priv = this;
    gCamCapability[cameraId]->version = CAM_HAL_V3;
    // TODO: hardcode for now until mctl add support for min_num_pp_bufs
    //TBD - To see if this hardcoding is needed. Check by printing if this is filled by mctl to 3
    gCamCapability[cameraId]->min_num_pp_bufs = 3;

    pthread_cond_init(&mRequestCond, NULL);
    mPendingRequest = 0;
    mCurrentRequestId = -1;
    pthread_mutex_init(&mMutex, NULL);

    for (size_t i = 0; i < CAMERA3_TEMPLATE_COUNT; i++)
        mDefaultMetadata[i] = NULL;

#ifdef HAS_MULTIMEDIA_HINTS
    if (hw_get_module(POWER_HARDWARE_MODULE_ID, (const hw_module_t **)&m_pPowerModule)) {
        ALOGE("%s: %s module not found", __func__, POWER_HARDWARE_MODULE_ID);
    }
#endif
}

/*===========================================================================
 * FUNCTION   : ~QCamera3HardwareInterface
 *
 * DESCRIPTION: destructor of QCamera3HardwareInterface
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3HardwareInterface::~QCamera3HardwareInterface()
{
    ALOGV("%s: E", __func__);
    /* We need to stop all streams before deleting any stream */

    // NOTE: 'camera3_stream_t *' objects are already freed at
    //        this stage by the framework
    for (List<stream_info_t *>::iterator it = mStreamInfo.begin();
        it != mStreamInfo.end(); it++) {
        QCamera3Channel *channel = (*it)->channel;
        if (channel) {
            channel->stop();
        }
    }
    if (mSupportChannel)
        mSupportChannel->stop();

    for (List<stream_info_t *>::iterator it = mStreamInfo.begin();
        it != mStreamInfo.end(); it++) {
        QCamera3Channel *channel = (*it)->channel;
        if (channel)
            delete channel;
        free (*it);
    }
    if (mSupportChannel) {
        delete mSupportChannel;
        mSupportChannel = NULL;
    }

    mPictureChannel = NULL;

    /* Clean up all channels */
    if (mCameraInitialized) {
        if (mMetadataChannel) {
            mMetadataChannel->stop();
            delete mMetadataChannel;
            mMetadataChannel = NULL;
        }
        deinitParameters();
    }

    if (mCameraOpened)
        closeCamera();

    mPendingBuffersMap.mPendingBufferList.clear();
    mPendingRequestsList.clear();

    for (size_t i = 0; i < CAMERA3_TEMPLATE_COUNT; i++)
        if (mDefaultMetadata[i])
            free_camera_metadata(mDefaultMetadata[i]);

    pthread_cond_destroy(&mRequestCond);

    pthread_mutex_destroy(&mMutex);
    ALOGV("%s: X", __func__);
}

/*===========================================================================
 * FUNCTION   : openCamera
 *
 * DESCRIPTION: open camera
 *
 * PARAMETERS :
 *   @hw_device  : double ptr for camera device struct
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::openCamera(struct hw_device_t **hw_device)
{
    int rc = 0;
    pthread_mutex_lock(&mCameraSessionLock);
    if (mCameraSessionActive) {
        ALOGE("%s: multiple simultaneous camera instance not supported", __func__);
        pthread_mutex_unlock(&mCameraSessionLock);
        return -EUSERS;
    }

    if (mCameraOpened) {
        *hw_device = NULL;
        return PERMISSION_DENIED;
    }

    rc = openCamera();
    if (rc == 0) {
        *hw_device = &mCameraDevice.common;
        mCameraSessionActive = 1;
    } else
        *hw_device = NULL;

#ifdef HAS_MULTIMEDIA_HINTS
    if (rc == 0) {
        if (m_pPowerModule) {
            if (m_pPowerModule->powerHint) {
                m_pPowerModule->powerHint(m_pPowerModule, POWER_HINT_VIDEO_ENCODE,
                        (void *)"state=1");
            }
        }
    }
#endif
    pthread_mutex_unlock(&mCameraSessionLock);
    return rc;
}

/*===========================================================================
 * FUNCTION   : openCamera
 *
 * DESCRIPTION: open camera
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::openCamera()
{
    if (mCameraHandle) {
        ALOGE("Failure: Camera already opened");
        return ALREADY_EXISTS;
    }
    mCameraHandle = camera_open(mCameraId);
    if (!mCameraHandle) {
        ALOGE("camera_open failed.");
        return UNKNOWN_ERROR;
    }

    mCameraOpened = true;

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : closeCamera
 *
 * DESCRIPTION: close camera
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::closeCamera()
{
    int rc = NO_ERROR;

    rc = mCameraHandle->ops->close_camera(mCameraHandle->camera_handle);
    mCameraHandle = NULL;
    mCameraOpened = false;

#ifdef HAS_MULTIMEDIA_HINTS
    if (rc == NO_ERROR) {
        if (m_pPowerModule) {
            if (m_pPowerModule->powerHint) {
                if(mHdrHint == true) {
                    m_pPowerModule->powerHint(m_pPowerModule, POWER_HINT_VIDEO_ENCODE,
                            (void *)"state=3");
                    mHdrHint = false;
                }
                m_pPowerModule->powerHint(m_pPowerModule, POWER_HINT_VIDEO_ENCODE,
                        (void *)"state=0");
            }
        }
    }
#endif

    return rc;
}

/*===========================================================================
 * FUNCTION   : initialize
 *
 * DESCRIPTION: Initialize frameworks callback functions
 *
 * PARAMETERS :
 *   @callback_ops : callback function to frameworks
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::initialize(
        const struct camera3_callback_ops *callback_ops)
{
    int rc;

    pthread_mutex_lock(&mMutex);

    rc = initParameters();
    if (rc < 0) {
        ALOGE("%s: initParamters failed %d", __func__, rc);
       goto err1;
    }
    mCallbackOps = callback_ops;

    pthread_mutex_unlock(&mMutex);
    mCameraInitialized = true;
    return 0;

err1:
    pthread_mutex_unlock(&mMutex);
    return rc;
}

/*===========================================================================
 * FUNCTION   : validateStreamDimensions
 *
 * DESCRIPTION: Check if the configuration requested are those advertised
 *
 * PARAMETERS :
 *   @stream_list : streams to be configured
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::validateStreamDimensions(
        camera3_stream_configuration_t *streamList)
{
    int rc = NO_ERROR;
    int32_t available_processed_sizes[MAX_SIZES_CNT * 2];

    /*
    * Loop through all streams requested in configuration
    * Check if unsupported sizes have been requested on any of them
    */
    for (size_t j = 0; j < streamList->num_streams; j++){
        bool sizeFound = false;
        camera3_stream_t *newStream = streamList->streams[j];

        /*
        * Sizes are different for each type of stream format check against
        * appropriate table.
        */
        switch (newStream->format) {
            case ANDROID_SCALER_AVAILABLE_FORMATS_RAW16:
            case ANDROID_SCALER_AVAILABLE_FORMATS_RAW_OPAQUE:
            case HAL_PIXEL_FORMAT_RAW10:
                for (int i = 0;
                      i < gCamCapability[mCameraId]->supported_raw_dim_cnt; i++){
                    if (gCamCapability[mCameraId]->raw_dim[i].width
                            == (int32_t) newStream->width
                        && gCamCapability[mCameraId]->raw_dim[i].height
                            == (int32_t) newStream->height) {
                        sizeFound = true;
                    }
                }
                break;
            case HAL_PIXEL_FORMAT_BLOB:
                for (int i = 0;
                  i < gCamCapability[mCameraId]->picture_sizes_tbl_cnt;i++){
                    if ((int32_t)(newStream->width) ==
                        gCamCapability[mCameraId]
                            ->picture_sizes_tbl[i].width
                    && (int32_t)(newStream->height) ==
                        gCamCapability[mCameraId]
                            ->picture_sizes_tbl[i].height){
                    sizeFound = true;
                    break;
                    }
                }
                break;

            case HAL_PIXEL_FORMAT_YCbCr_420_888:
            case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
            default:
                /* ZSL stream will be full active array size validate that*/
                if (newStream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
                    if ((int32_t)(newStream->width) ==
                        gCamCapability[mCameraId]->active_array_size.width
                        && (int32_t)(newStream->height)  ==
                        gCamCapability[mCameraId]->active_array_size.height) {
                        sizeFound = true;
                    }
                    /* We could potentially break here to enforce ZSL stream
                     * set from frameworks always has full active array size
                     * but it is not clear from spec if framework will always
                     * follow that, also we have logic to override to full array
                     * size, so keeping this logic lenient at the moment.
                     */
                }

                /* Non ZSL stream still need to conform to advertised sizes*/
                for (int i = 0;
                  i < gCamCapability[mCameraId]->picture_sizes_tbl_cnt;i++){
                    if ((int32_t)(newStream->width) ==
                        gCamCapability[mCameraId]
                            ->picture_sizes_tbl[i].width
                    && (int32_t)(newStream->height) ==
                        gCamCapability[mCameraId]
                            ->picture_sizes_tbl[i].height){
                    sizeFound = true;
                    break;
                    }
                }
                break;
        } /* End of switch(newStream->format) */

        /* We error out even if a single stream has unsupported size set */
        if (!sizeFound) {
            ALOGE("%s: Error: Unsupported size of  %d x %d requested for stream"
                  "type:%d", __func__, newStream->width, newStream->height,
                  newStream->format);
            rc = -EINVAL;
            break;
        }
    } /* End of for each stream */
    return rc;
}

/*===========================================================================
 * FUNCTION   : configureStreams
 *
 * DESCRIPTION: Reset HAL camera device processing pipeline and set up new input
 *              and output streams.
 *
 * PARAMETERS :
 *   @stream_list : streams to be configured
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::configureStreams(
        camera3_stream_configuration_t *streamList)
{
    int rc = 0;

    // Sanity check stream_list
    if (streamList == NULL) {
        ALOGE("%s: NULL stream configuration", __func__);
        return BAD_VALUE;
    }
    if (streamList->streams == NULL) {
        ALOGE("%s: NULL stream list", __func__);
        return BAD_VALUE;
    }

    if (streamList->num_streams < 1) {
        ALOGE("%s: Bad number of streams requested: %d", __func__,
                streamList->num_streams);
        return BAD_VALUE;
    }

    rc = validateStreamDimensions(streamList);
    if (rc != NO_ERROR) {
        ALOGE("%s: Invalid stream configuration requested!", __func__);
        return rc;
    }

    /* first invalidate all the steams in the mStreamList
     * if they appear again, they will be validated */
    for (List<stream_info_t*>::iterator it = mStreamInfo.begin();
            it != mStreamInfo.end(); it++) {
        QCamera3Channel *channel = (QCamera3Channel*)(*it)->stream->priv;
        channel->stop();
        (*it)->status = INVALID;
    }
    if (mSupportChannel)
        mSupportChannel->stop();
    if (mMetadataChannel) {
        /* If content of mStreamInfo is not 0, there is metadata stream */
        mMetadataChannel->stop();
    }

#ifdef HAS_MULTIMEDIA_HINTS
    if(mHdrHint == true) {
        if (m_pPowerModule) {
            if (m_pPowerModule->powerHint) {
                m_pPowerModule->powerHint(m_pPowerModule, POWER_HINT_VIDEO_ENCODE,
                        (void *)"state=3");
                mHdrHint = false;
            }
        }
    }
#endif

    pthread_mutex_lock(&mMutex);

    bool isZsl = false;
    camera3_stream_t *inputStream = NULL;
    camera3_stream_t *jpegStream = NULL;
    cam_stream_size_info_t stream_config_info;

    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];
        ALOGD("%s: newStream type = %d, stream format = %d stream size : %d x %d",
                __func__, newStream->stream_type, newStream->format,
                 newStream->width, newStream->height);
        //if the stream is in the mStreamList validate it
        bool stream_exists = false;
        for (List<stream_info_t*>::iterator it=mStreamInfo.begin();
                it != mStreamInfo.end(); it++) {
            if ((*it)->stream == newStream) {
                QCamera3Channel *channel =
                    (QCamera3Channel*)(*it)->stream->priv;
                stream_exists = true;
                delete channel;
                (*it)->status = VALID;
                (*it)->stream->priv = NULL;
                (*it)->channel = NULL;
            }
        }
        if (!stream_exists) {
            //new stream
            stream_info_t* stream_info;
            stream_info = (stream_info_t* )malloc(sizeof(stream_info_t));
            stream_info->stream = newStream;
            stream_info->status = VALID;
            stream_info->channel = NULL;
            mStreamInfo.push_back(stream_info);
        }
        if (newStream->stream_type == CAMERA3_STREAM_INPUT
                || newStream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL ) {
            if (inputStream != NULL) {
                ALOGE("%s: Multiple input streams requested!", __func__);
                pthread_mutex_unlock(&mMutex);
                return BAD_VALUE;
            }
            inputStream = newStream;
        }
        if (newStream->format == HAL_PIXEL_FORMAT_BLOB) {
            jpegStream = newStream;
        }
    }
    mInputStream = inputStream;

    cleanAndSortStreamInfo();
    if (mMetadataChannel) {
        delete mMetadataChannel;
        mMetadataChannel = NULL;
    }
    if (mSupportChannel) {
        delete mSupportChannel;
        mSupportChannel = NULL;
    }

    //Create metadata channel and initialize it
    mMetadataChannel = new QCamera3MetadataChannel(mCameraHandle->camera_handle,
                    mCameraHandle->ops, captureResultCb,
                    &gCamCapability[mCameraId]->padding_info, this);
    if (mMetadataChannel == NULL) {
        ALOGE("%s: failed to allocate metadata channel", __func__);
        rc = -ENOMEM;
        pthread_mutex_unlock(&mMutex);
        return rc;
    }
    rc = mMetadataChannel->initialize();
    if (rc < 0) {
        ALOGE("%s: metadata channel initialization failed", __func__);
        delete mMetadataChannel;
        mMetadataChannel = NULL;
        pthread_mutex_unlock(&mMutex);
        return rc;
    }

    /* Create dummy stream if there is one single raw stream */
    if (streamList->num_streams == 1 &&
            (streamList->streams[0]->format == HAL_PIXEL_FORMAT_RAW_OPAQUE ||
            streamList->streams[0]->format == HAL_PIXEL_FORMAT_RAW16)) {
        mSupportChannel = new QCamera3SupportChannel(
                mCameraHandle->camera_handle,
                mCameraHandle->ops,
                &gCamCapability[mCameraId]->padding_info,
                this);
        if (!mSupportChannel) {
            ALOGE("%s: dummy channel cannot be created", __func__);
            pthread_mutex_unlock(&mMutex);
            return -ENOMEM;
        }
    }

    /* Allocate channel objects for the requested streams */
    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];
        uint32_t stream_usage = newStream->usage;
        stream_config_info.stream_sizes[i].width = newStream->width;
        stream_config_info.stream_sizes[i].height = newStream->height;
        if (newStream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL &&
            newStream->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED && jpegStream){
            //for zsl stream the size is active array size
            isZsl = true;
            stream_config_info.stream_sizes[i].width =
                    gCamCapability[mCameraId]->active_array_size.width;
            stream_config_info.stream_sizes[i].height =
                    gCamCapability[mCameraId]->active_array_size.height;
            stream_config_info.type[i] = CAM_STREAM_TYPE_SNAPSHOT;
        } else {
           //for non zsl streams find out the format
           switch (newStream->format) {
           case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED :
              {
                 if (stream_usage & private_handle_t::PRIV_FLAGS_VIDEO_ENCODER) {
                    stream_config_info.type[i] = CAM_STREAM_TYPE_VIDEO;
                 } else {
                    stream_config_info.type[i] = CAM_STREAM_TYPE_PREVIEW;
                 }
              }
              break;
           case HAL_PIXEL_FORMAT_YCbCr_420_888:
              stream_config_info.type[i] = CAM_STREAM_TYPE_CALLBACK;
#ifdef HAS_MULTIMEDIA_HINTS
              if (m_pPowerModule) {
                  if (m_pPowerModule->powerHint) {
                      m_pPowerModule->powerHint(m_pPowerModule,
                          POWER_HINT_VIDEO_ENCODE, (void *)"state=2");
                      mHdrHint = true;
                  }
              }
#endif
              break;
           case HAL_PIXEL_FORMAT_BLOB:
              stream_config_info.type[i] = CAM_STREAM_TYPE_NON_ZSL_SNAPSHOT;
              break;
           case HAL_PIXEL_FORMAT_RAW_OPAQUE:
           case HAL_PIXEL_FORMAT_RAW16:
              stream_config_info.type[i] = CAM_STREAM_TYPE_RAW;
              break;
           default:
              stream_config_info.type[i] = CAM_STREAM_TYPE_DEFAULT;
              break;
           }
        }
        if (newStream->priv == NULL) {
            //New stream, construct channel
            switch (newStream->stream_type) {
            case CAMERA3_STREAM_INPUT:
                newStream->usage = GRALLOC_USAGE_HW_CAMERA_READ;
                break;
            case CAMERA3_STREAM_BIDIRECTIONAL:
                newStream->usage = GRALLOC_USAGE_HW_CAMERA_READ |
                    GRALLOC_USAGE_HW_CAMERA_WRITE;
                break;
            case CAMERA3_STREAM_OUTPUT:
                /* For video encoding stream, set read/write rarely
                 * flag so that they may be set to un-cached */
                if (newStream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
                    newStream->usage =
                         (GRALLOC_USAGE_SW_READ_RARELY |
                         GRALLOC_USAGE_SW_WRITE_RARELY |
                         GRALLOC_USAGE_HW_CAMERA_WRITE);
                else
                    newStream->usage = GRALLOC_USAGE_HW_CAMERA_WRITE;
                break;
            default:
                ALOGE("%s: Invalid stream_type %d", __func__, newStream->stream_type);
                break;
            }

            if (newStream->stream_type == CAMERA3_STREAM_OUTPUT ||
                    newStream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
                QCamera3Channel *channel = NULL;
                switch (newStream->format) {
                case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
                case HAL_PIXEL_FORMAT_YCbCr_420_888:
                    newStream->max_buffers = QCamera3RegularChannel::kMaxBuffers;
                    channel = new QCamera3RegularChannel(mCameraHandle->camera_handle,
                            mCameraHandle->ops, captureResultCb,
                            &gCamCapability[mCameraId]->padding_info,
                            this,
                            newStream,
                            (cam_stream_type_t) stream_config_info.type[i]);
                    if (channel == NULL) {
                        ALOGE("%s: allocation of channel failed", __func__);
                        pthread_mutex_unlock(&mMutex);
                        return -ENOMEM;
                    }

                    newStream->priv = channel;
                    break;
                case HAL_PIXEL_FORMAT_RAW_OPAQUE:
                case HAL_PIXEL_FORMAT_RAW16:
                    newStream->max_buffers = QCamera3RawChannel::kMaxBuffers;
                    mRawChannel = new QCamera3RawChannel(
                            mCameraHandle->camera_handle,
                            mCameraHandle->ops, captureResultCb,
                            &gCamCapability[mCameraId]->padding_info,
                            this, newStream, (newStream->format == HAL_PIXEL_FORMAT_RAW16));
                    if (mRawChannel == NULL) {
                        ALOGE("%s: allocation of raw channel failed", __func__);
                        pthread_mutex_unlock(&mMutex);
                        return -ENOMEM;
                    }

                    newStream->priv = (QCamera3Channel*)mRawChannel;
                    break;
                case HAL_PIXEL_FORMAT_BLOB:
                    newStream->max_buffers = QCamera3PicChannel::kMaxBuffers;
                    mPictureChannel = new QCamera3PicChannel(mCameraHandle->camera_handle,
                            mCameraHandle->ops, captureResultCb,
                            &gCamCapability[mCameraId]->padding_info, this, newStream);
                    if (mPictureChannel == NULL) {
                        ALOGE("%s: allocation of channel failed", __func__);
                        pthread_mutex_unlock(&mMutex);
                        return -ENOMEM;
                    }
                    newStream->priv = (QCamera3Channel*)mPictureChannel;
                    break;

                default:
                    ALOGE("%s: not a supported format 0x%x", __func__, newStream->format);
                    break;
                }
            }

            for (List<stream_info_t*>::iterator it=mStreamInfo.begin();
                    it != mStreamInfo.end(); it++) {
                if ((*it)->stream == newStream) {
                    (*it)->channel = (QCamera3Channel*) newStream->priv;
                    break;
                }
            }
        } else {
            // Channel already exists for this stream
            // Do nothing for now
        }
    }

    if (isZsl)
        mPictureChannel->overrideYuvSize(
                gCamCapability[mCameraId]->active_array_size.width,
                gCamCapability[mCameraId]->active_array_size.height);

    int32_t hal_version = CAM_HAL_V3;
    stream_config_info.num_streams = streamList->num_streams;
    if (mSupportChannel) {
        stream_config_info.stream_sizes[stream_config_info.num_streams] =
                QCamera3SupportChannel::kDim;
        stream_config_info.type[stream_config_info.num_streams] =
                CAM_STREAM_TYPE_CALLBACK;
        stream_config_info.num_streams++;
    }

    // settings/parameters don't carry over for new configureStreams
    memset(mParameters, 0, sizeof(metadata_buffer_t));

    mParameters->first_flagged_entry = CAM_INTF_PARM_MAX;
    AddSetMetaEntryToBatch(mParameters, CAM_INTF_PARM_HAL_VERSION,
                sizeof(hal_version), &hal_version);

    AddSetMetaEntryToBatch(mParameters, CAM_INTF_META_STREAM_INFO,
                sizeof(stream_config_info), &stream_config_info);

    mCameraHandle->ops->set_parms(mCameraHandle->camera_handle, mParameters);

    /* Initialize mPendingRequestInfo and mPendnigBuffersMap */
    mPendingRequestsList.clear();
    mPendingFrameDropList.clear();
    // Initialize/Reset the pending buffers list
    mPendingBuffersMap.num_buffers = 0;
    mPendingBuffersMap.mPendingBufferList.clear();

    mFirstRequest = true;

    //Get min frame duration for this streams configuration
    deriveMinFrameDuration();

    pthread_mutex_unlock(&mMutex);
    return rc;
}

/*===========================================================================
 * FUNCTION   : validateCaptureRequest
 *
 * DESCRIPTION: validate a capture request from camera service
 *
 * PARAMETERS :
 *   @request : request from framework to process
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::validateCaptureRequest(
                    camera3_capture_request_t *request)
{
    ssize_t idx = 0;
    const camera3_stream_buffer_t *b;
    CameraMetadata meta;

    /* Sanity check the request */
    if (request == NULL) {
        ALOGE("%s: NULL capture request", __func__);
        return BAD_VALUE;
    }

    if (request->settings == NULL && mFirstRequest) {
        /*settings cannot be null for the first request*/
        return BAD_VALUE;
    }

    uint32_t frameNumber = request->frame_number;
    if (request->input_buffer != NULL &&
            request->input_buffer->stream != mInputStream) {
        ALOGE("%s: Request %d: Input buffer not from input stream!",
                __FUNCTION__, frameNumber);
        return BAD_VALUE;
    }
    if (request->num_output_buffers < 1 || request->output_buffers == NULL) {
        ALOGE("%s: Request %d: No output buffers provided!",
                __FUNCTION__, frameNumber);
        return BAD_VALUE;
    }
    if (request->input_buffer != NULL) {
        b = request->input_buffer;
        QCamera3Channel *channel =
            static_cast<QCamera3Channel*>(b->stream->priv);
        if (channel == NULL) {
            ALOGE("%s: Request %d: Buffer %d: Unconfigured stream!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            ALOGE("%s: Request %d: Buffer %d: Status not OK!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            ALOGE("%s: Request %d: Buffer %d: Has a release fence!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            ALOGE("%s: Request %d: Buffer %d: NULL buffer handle!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
    }

    // Validate all buffers
    b = request->output_buffers;
    do {
        QCamera3Channel *channel =
                static_cast<QCamera3Channel*>(b->stream->priv);
        if (channel == NULL) {
            ALOGE("%s: Request %d: Buffer %d: Unconfigured stream!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            ALOGE("%s: Request %d: Buffer %d: Status not OK!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            ALOGE("%s: Request %d: Buffer %d: Has a release fence!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            ALOGE("%s: Request %d: Buffer %d: NULL buffer handle!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        idx++;
        b = request->output_buffers + idx;
    } while (idx < (ssize_t)request->num_output_buffers);

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : deriveMinFrameDuration
 *
 * DESCRIPTION: derive mininum processed, jpeg, and raw frame durations based
 *              on currently configured streams.
 *
 * PARAMETERS : NONE
 *
 * RETURN     : NONE
 *
 *==========================================================================*/
void QCamera3HardwareInterface::deriveMinFrameDuration()
{
    int32_t maxJpegDim, maxProcessedDim, maxRawDim;

    maxJpegDim = 0;
    maxProcessedDim = 0;
    maxRawDim = 0;

    // Figure out maximum jpeg, processed, and raw dimensions
    for (List<stream_info_t*>::iterator it = mStreamInfo.begin();
        it != mStreamInfo.end(); it++) {

        // Input stream doesn't have valid stream_type
        if ((*it)->stream->stream_type == CAMERA3_STREAM_INPUT)
            continue;

        int32_t dimension = (*it)->stream->width * (*it)->stream->height;
        if ((*it)->stream->format == HAL_PIXEL_FORMAT_BLOB) {
            if (dimension > maxJpegDim)
                maxJpegDim = dimension;
        } else if ((*it)->stream->format == HAL_PIXEL_FORMAT_RAW_OPAQUE ||
                (*it)->stream->format == HAL_PIXEL_FORMAT_RAW16) {
            if (dimension > maxRawDim)
                maxRawDim = dimension;
        } else {
            if (dimension > maxProcessedDim)
                maxProcessedDim = dimension;
        }
    }

    //Assume all jpeg dimensions are in processed dimensions.
    if (maxJpegDim > maxProcessedDim)
        maxProcessedDim = maxJpegDim;
    //Find the smallest raw dimension that is greater or equal to jpeg dimension
    if (maxProcessedDim > maxRawDim) {
        maxRawDim = INT32_MAX;
        for (int i = 0; i < gCamCapability[mCameraId]->supported_raw_dim_cnt;
            i++) {

            int32_t dimension =
                gCamCapability[mCameraId]->raw_dim[i].width *
                gCamCapability[mCameraId]->raw_dim[i].height;

            if (dimension >= maxProcessedDim && dimension < maxRawDim)
                maxRawDim = dimension;
        }
    }

    //Find minimum durations for processed, jpeg, and raw
    for (int i = 0; i < gCamCapability[mCameraId]->supported_raw_dim_cnt;
            i++) {
        if (maxRawDim == gCamCapability[mCameraId]->raw_dim[i].width *
                gCamCapability[mCameraId]->raw_dim[i].height) {
            mMinRawFrameDuration = gCamCapability[mCameraId]->raw_min_duration[i];
            break;
        }
    }
    for (int i = 0; i < gCamCapability[mCameraId]->picture_sizes_tbl_cnt; i++) {
        if (maxProcessedDim ==
            gCamCapability[mCameraId]->picture_sizes_tbl[i].width *
            gCamCapability[mCameraId]->picture_sizes_tbl[i].height) {
            mMinProcessedFrameDuration = gCamCapability[mCameraId]->picture_min_duration[i];
            mMinJpegFrameDuration = gCamCapability[mCameraId]->picture_min_duration[i];
            break;
        }
    }
}

/*===========================================================================
 * FUNCTION   : getMinFrameDuration
 *
 * DESCRIPTION: get minimum frame draution based on the current maximum frame durations
 *              and current request configuration.
 *
 * PARAMETERS : @request: requset sent by the frameworks
 *
 * RETURN     : min farme duration for a particular request
 *
 *==========================================================================*/
int64_t QCamera3HardwareInterface::getMinFrameDuration(const camera3_capture_request_t *request)
{
    bool hasJpegStream = false;
    bool hasRawStream = false;
    for (uint32_t i = 0; i < request->num_output_buffers; i ++) {
        const camera3_stream_t *stream = request->output_buffers[i].stream;
        if (stream->format == HAL_PIXEL_FORMAT_BLOB)
            hasJpegStream = true;
        else if (stream->format == HAL_PIXEL_FORMAT_RAW_OPAQUE ||
                stream->format == HAL_PIXEL_FORMAT_RAW16)
            hasRawStream = true;
    }

    if (!hasJpegStream)
        return MAX(mMinRawFrameDuration, mMinProcessedFrameDuration);
    else
        return MAX(MAX(mMinRawFrameDuration, mMinProcessedFrameDuration), mMinJpegFrameDuration);
}

/*===========================================================================
 * FUNCTION   : handleMetadataWithLock
 *
 * DESCRIPTION: Handles metadata buffer callback with mMutex lock held.
 *
 * PARAMETERS : @metadata_buf: metadata buffer
 *
 * RETURN     :
 *
 *==========================================================================*/
void QCamera3HardwareInterface::handleMetadataWithLock(
    mm_camera_super_buf_t *metadata_buf)
{
    metadata_buffer_t *metadata = (metadata_buffer_t *)metadata_buf->bufs[0]->buffer;
    int32_t frame_number_valid = *(int32_t *)
        POINTER_OF(CAM_INTF_META_FRAME_NUMBER_VALID, metadata);
    uint32_t pending_requests = *(uint32_t *)POINTER_OF(
        CAM_INTF_META_PENDING_REQUESTS, metadata);
    uint32_t frame_number = *(uint32_t *)
        POINTER_OF(CAM_INTF_META_FRAME_NUMBER, metadata);
    const struct timeval *tv = (const struct timeval *)
        POINTER_OF(CAM_INTF_META_SENSOR_TIMESTAMP, metadata);
    nsecs_t capture_time = (nsecs_t)tv->tv_sec * NSEC_PER_SEC +
        tv->tv_usec * NSEC_PER_USEC;
    cam_frame_dropped_t cam_frame_drop = *(cam_frame_dropped_t *)
        POINTER_OF(CAM_INTF_META_FRAME_DROPPED, metadata);

    int32_t urgent_frame_number_valid = *(int32_t *)
        POINTER_OF(CAM_INTF_META_URGENT_FRAME_NUMBER_VALID, metadata);
    uint32_t urgent_frame_number = *(uint32_t *)
        POINTER_OF(CAM_INTF_META_URGENT_FRAME_NUMBER, metadata);

    if (urgent_frame_number_valid) {
        ALOGV("%s: valid urgent frame_number = %d, capture_time = %lld",
          __func__, urgent_frame_number, capture_time);

        //Recieved an urgent Frame Number, handle it
        //using partial results
        for (List<PendingRequestInfo>::iterator i =
            mPendingRequestsList.begin(); i != mPendingRequestsList.end(); i++) {
            camera3_notify_msg_t notify_msg;
            ALOGV("%s: Iterator Frame = %d urgent frame = %d",
                __func__, i->frame_number, urgent_frame_number);

            if (i->frame_number < urgent_frame_number &&
                i->bNotified == 0) {
                notify_msg.type = CAMERA3_MSG_SHUTTER;
                notify_msg.message.shutter.frame_number = i->frame_number;
                notify_msg.message.shutter.timestamp = capture_time -
                    (urgent_frame_number - i->frame_number) * NSEC_PER_33MSEC;
                mCallbackOps->notify(mCallbackOps, &notify_msg);
                i->timestamp = notify_msg.message.shutter.timestamp;
                i->bNotified = 1;
                ALOGV("%s: Support notification !!!! notify frame_number = %d, capture_time = %lld",
                    __func__, i->frame_number, notify_msg.message.shutter.timestamp);
            }

            if (i->frame_number == urgent_frame_number) {

                camera3_capture_result_t result;
                memset(&result, 0, sizeof(camera3_capture_result_t));

                // Send shutter notify to frameworks
                notify_msg.type = CAMERA3_MSG_SHUTTER;
                notify_msg.message.shutter.frame_number = i->frame_number;
                notify_msg.message.shutter.timestamp = capture_time;
                mCallbackOps->notify(mCallbackOps, &notify_msg);

                i->timestamp = capture_time;
                i->bNotified = 1;
                i->partial_result_cnt++;
                // Extract 3A metadata
                result.result =
                    translateCbUrgentMetadataToResultMetadata(metadata);
                // Populate metadata result
                result.frame_number = urgent_frame_number;
                result.num_output_buffers = 0;
                result.output_buffers = NULL;
                result.partial_result = i->partial_result_cnt;

                mCallbackOps->process_capture_result(mCallbackOps, &result);
                ALOGV("%s: urgent frame_number = %d, capture_time = %lld",
                     __func__, result.frame_number, capture_time);
                free_camera_metadata((camera_metadata_t *)result.result);
                break;
            }
        }
    }

    if (!frame_number_valid) {
        ALOGV("%s: Not a valid normal frame number, used as SOF only", __func__);
        mMetadataChannel->bufDone(metadata_buf);
        free(metadata_buf);
        goto done_metadata;
    }
    ALOGV("%s: valid normal frame_number = %d, capture_time = %lld", __func__,
            frame_number, capture_time);

    // Go through the pending requests info and send shutter/results to frameworks
    for (List<PendingRequestInfo>::iterator i = mPendingRequestsList.begin();
        i != mPendingRequestsList.end() && i->frame_number <= frame_number;) {
        camera3_capture_result_t result;
        memset(&result, 0, sizeof(camera3_capture_result_t));
        ALOGV("%s: frame_number in the list is %d", __func__, i->frame_number);

        i->partial_result_cnt++;
        result.partial_result = i->partial_result_cnt;

        // Flush out all entries with less or equal frame numbers.
        mPendingRequest--;

        // Check whether any stream buffer corresponding to this is dropped or not
        // If dropped, then notify ERROR_BUFFER for the corresponding stream and
        // buffer with CAMERA3_BUFFER_STATUS_ERROR
        if (cam_frame_drop.frame_dropped) {
            camera3_notify_msg_t notify_msg;
            for (List<RequestedBufferInfo>::iterator j = i->buffers.begin();
                    j != i->buffers.end(); j++) {
                QCamera3Channel *channel = (QCamera3Channel *)j->stream->priv;
                uint32_t streamID = channel->getStreamID(channel->getStreamTypeMask());
                for (uint32_t k=0; k<cam_frame_drop.cam_stream_ID.num_streams; k++) {
                  if (streamID == cam_frame_drop.cam_stream_ID.streamID[k]) {
                      // Send Error notify to frameworks with CAMERA3_MSG_ERROR_BUFFER
                      ALOGV("%s: Start of reporting error frame#=%d, streamID=%d",
                             __func__, i->frame_number, streamID);
                      notify_msg.type = CAMERA3_MSG_ERROR;
                      notify_msg.message.error.frame_number = i->frame_number;
                      notify_msg.message.error.error_code = CAMERA3_MSG_ERROR_BUFFER ;
                      notify_msg.message.error.error_stream = j->stream;
                      mCallbackOps->notify(mCallbackOps, &notify_msg);
                      ALOGV("%s: End of reporting error frame#=%d, streamID=%d",
                             __func__, i->frame_number, streamID);
                      PendingFrameDropInfo PendingFrameDrop;
                      PendingFrameDrop.frame_number=i->frame_number;
                      PendingFrameDrop.stream_ID = streamID;
                      // Add the Frame drop info to mPendingFrameDropList
                      mPendingFrameDropList.push_back(PendingFrameDrop);
                  }
                }
            }
        }

        // Send empty metadata with already filled buffers for dropped metadata
        // and send valid metadata with already filled buffers for current metadata
        if (i->frame_number < frame_number) {
            CameraMetadata dummyMetadata;
            dummyMetadata.update(ANDROID_SENSOR_TIMESTAMP,
                    &i->timestamp, 1);
            dummyMetadata.update(ANDROID_REQUEST_ID,
                    &(i->request_id), 1);
            result.result = dummyMetadata.release();
        } else {
            uint8_t bufferStalled = *((uint8_t *)
                    POINTER_OF(CAM_INTF_META_FRAMES_STALLED, metadata));

            if (bufferStalled) {
                result.result = NULL; //Metadata should not be sent in this case
                camera3_notify_msg_t notify_msg;
                memset(&notify_msg, 0, sizeof(camera3_notify_msg_t));
                notify_msg.type = CAMERA3_MSG_ERROR;
                notify_msg.message.error.frame_number = i->frame_number;
                notify_msg.message.error.error_code = CAMERA3_MSG_ERROR_REQUEST;
                notify_msg.message.error.error_stream = NULL;
                ALOGE("%s: Buffer stall observed reporting error", __func__);
                mCallbackOps->notify(mCallbackOps, &notify_msg);
            } else {
                result.result = translateFromHalMetadata(metadata,
                        i->timestamp, i->request_id, i->jpegMetadata,
                        i->pipeline_depth);
            }

            if (i->blob_request) {
                {
                    //Dump tuning metadata if enabled and available
                    char prop[PROPERTY_VALUE_MAX];
                    memset(prop, 0, sizeof(prop));
                    property_get("persist.camera.dumpmetadata", prop, "0");
                    int32_t enabled = atoi(prop);
                    if (enabled && metadata->is_tuning_params_valid) {
                        dumpMetadataToFile(metadata->tuning_params,
                               mMetaFrameCount,
                               enabled,
                               "Snapshot",
                               frame_number);
                    }
                }

                //If it is a blob request then send the metadata to the picture channel
                metadata_buffer_t *reproc_meta =
                        (metadata_buffer_t *)malloc(sizeof(metadata_buffer_t));
                if (reproc_meta == NULL) {
                    ALOGE("%s: Failed to allocate memory for reproc data.", __func__);
                    goto done_metadata;
                }
                *reproc_meta = *metadata;
                mPictureChannel->queueReprocMetadata(reproc_meta);
            }
            // Return metadata buffer
            mMetadataChannel->bufDone(metadata_buf);
            free(metadata_buf);
        }
        if (!result.result) {
            ALOGE("%s: metadata is NULL", __func__);
        }
        result.frame_number = i->frame_number;
        result.num_output_buffers = 0;
        result.output_buffers = NULL;
        for (List<RequestedBufferInfo>::iterator j = i->buffers.begin();
                    j != i->buffers.end(); j++) {
            if (j->buffer) {
                result.num_output_buffers++;
            }
        }

        if (result.num_output_buffers > 0) {
            camera3_stream_buffer_t *result_buffers =
                new camera3_stream_buffer_t[result.num_output_buffers];
            if (!result_buffers) {
                ALOGE("%s: Fatal error: out of memory", __func__);
            }
            size_t result_buffers_idx = 0;
            for (List<RequestedBufferInfo>::iterator j = i->buffers.begin();
                    j != i->buffers.end(); j++) {
                if (j->buffer) {
                    for (List<PendingFrameDropInfo>::iterator m = mPendingFrameDropList.begin();
                            m != mPendingFrameDropList.end(); m++) {
                        QCamera3Channel *channel = (QCamera3Channel *)j->buffer->stream->priv;
                        uint32_t streamID = channel->getStreamID(channel->getStreamTypeMask());
                        if((m->stream_ID==streamID) && (m->frame_number==frame_number)) {
                            j->buffer->status=CAMERA3_BUFFER_STATUS_ERROR;
                            ALOGV("%s: Stream STATUS_ERROR frame_number=%d, streamID=%d",
                                  __func__, frame_number, streamID);
                            m = mPendingFrameDropList.erase(m);
                            break;
                        }
                    }

                    for (List<PendingBufferInfo>::iterator k =
                      mPendingBuffersMap.mPendingBufferList.begin();
                      k != mPendingBuffersMap.mPendingBufferList.end(); k++) {
                      if (k->buffer == j->buffer->buffer) {
                        ALOGV("%s: Found buffer %p in pending buffer List "
                              "for frame %d, Take it out!!", __func__,
                               k->buffer, k->frame_number);
                        mPendingBuffersMap.num_buffers--;
                        k = mPendingBuffersMap.mPendingBufferList.erase(k);
                        break;
                      }
                    }

                    result_buffers[result_buffers_idx++] = *(j->buffer);
                    free(j->buffer);
                    j->buffer = NULL;
                }
            }
            result.output_buffers = result_buffers;
            mCallbackOps->process_capture_result(mCallbackOps, &result);
            ALOGV("%s: meta frame_number = %d, capture_time = %lld",
                    __func__, result.frame_number, i->timestamp);
            free_camera_metadata((camera_metadata_t *)result.result);
            delete[] result_buffers;
        } else {
            mCallbackOps->process_capture_result(mCallbackOps, &result);
            ALOGV("%s: meta frame_number = %d, capture_time = %lld",
                        __func__, result.frame_number, i->timestamp);
            free_camera_metadata((camera_metadata_t *)result.result);
        }
        // erase the element from the list
        i = mPendingRequestsList.erase(i);
    }

done_metadata:
    for (List<PendingRequestInfo>::iterator i = mPendingRequestsList.begin();
        i != mPendingRequestsList.end() ;i++) {
        i->pipeline_depth++;
    }
    if (!pending_requests)
        unblockRequestIfNecessary();

}

/*===========================================================================
 * FUNCTION   : handleBufferWithLock
 *
 * DESCRIPTION: Handles image buffer callback with mMutex lock held.
 *
 * PARAMETERS : @buffer: image buffer for the callback
 *              @frame_number: frame number of the image buffer
 *
 * RETURN     :
 *
 *==========================================================================*/
void QCamera3HardwareInterface::handleBufferWithLock(
    camera3_stream_buffer_t *buffer, uint32_t frame_number)
{
    // If the frame number doesn't exist in the pending request list,
    // directly send the buffer to the frameworks, and update pending buffers map
    // Otherwise, book-keep the buffer.
    List<PendingRequestInfo>::iterator i = mPendingRequestsList.begin();
    while (i != mPendingRequestsList.end() && i->frame_number != frame_number){
        i++;
    }
    if (i == mPendingRequestsList.end()) {
        // Verify all pending requests frame_numbers are greater
        for (List<PendingRequestInfo>::iterator j = mPendingRequestsList.begin();
                j != mPendingRequestsList.end(); j++) {
            if (j->frame_number < frame_number) {
                ALOGE("%s: Error: pending frame number %d is smaller than %d",
                        __func__, j->frame_number, frame_number);
            }
        }
        camera3_capture_result_t result;
        memset(&result, 0, sizeof(camera3_capture_result_t));
        result.result = NULL;
        result.frame_number = frame_number;
        result.num_output_buffers = 1;
        result.partial_result = 0;
        for (List<PendingFrameDropInfo>::iterator m = mPendingFrameDropList.begin();
                m != mPendingFrameDropList.end(); m++) {
            QCamera3Channel *channel = (QCamera3Channel *)buffer->stream->priv;
            uint32_t streamID = channel->getStreamID(channel->getStreamTypeMask());
            if((m->stream_ID==streamID) && (m->frame_number==frame_number)) {
                buffer->status=CAMERA3_BUFFER_STATUS_ERROR;
                ALOGV("%s: Stream STATUS_ERROR frame_number=%d, streamID=%d",
                        __func__, frame_number, streamID);
                m = mPendingFrameDropList.erase(m);
                break;
            }
        }
        result.output_buffers = buffer;
        ALOGV("%s: result frame_number = %d, buffer = %p",
                __func__, frame_number, buffer->buffer);

        for (List<PendingBufferInfo>::iterator k =
                mPendingBuffersMap.mPendingBufferList.begin();
                k != mPendingBuffersMap.mPendingBufferList.end(); k++ ) {
            if (k->buffer == buffer->buffer) {
                ALOGV("%s: Found Frame buffer, take it out from list",
                        __func__);

                mPendingBuffersMap.num_buffers--;
                k = mPendingBuffersMap.mPendingBufferList.erase(k);
                break;
            }
        }
        ALOGV("%s: mPendingBuffersMap.num_buffers = %d",
            __func__, mPendingBuffersMap.num_buffers);

        mCallbackOps->process_capture_result(mCallbackOps, &result);
    } else {
        if (i->input_buffer_present) {
            camera3_capture_result result;
            memset(&result, 0, sizeof(camera3_capture_result_t));
            result.result = NULL;
            result.frame_number = frame_number;
            result.num_output_buffers = 1;
            result.output_buffers = buffer;
            result.partial_result = 0;
            mCallbackOps->process_capture_result(mCallbackOps, &result);
            i = mPendingRequestsList.erase(i);
            mPendingRequest--;
        } else {
            for (List<RequestedBufferInfo>::iterator j = i->buffers.begin();
                j != i->buffers.end(); j++) {
                if (j->stream == buffer->stream) {
                    if (j->buffer != NULL) {
                        ALOGE("%s: Error: buffer is already set", __func__);
                    } else {
                        j->buffer = (camera3_stream_buffer_t *)malloc(
                            sizeof(camera3_stream_buffer_t));
                        *(j->buffer) = *buffer;
                        ALOGV("%s: cache buffer %p at result frame_number %d",
                            __func__, buffer, frame_number);
                    }
                }
            }
        }
    }
}

/*===========================================================================
 * FUNCTION   : unblockRequestIfNecessary
 *
 * DESCRIPTION: Unblock capture_request if max_buffer hasn't been reached. Note
 *              that mMutex is held when this function is called.
 *
 * PARAMETERS :
 *
 * RETURN     :
 *
 *==========================================================================*/
void QCamera3HardwareInterface::unblockRequestIfNecessary()
{
   // Unblock process_capture_request
   pthread_cond_signal(&mRequestCond);
}

/*===========================================================================
 * FUNCTION   : registerStreamBuffers
 *
 * DESCRIPTION: Register buffers for a given stream with the HAL device.
 *
 * PARAMETERS :
 *   @stream_list : streams to be configured
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::registerStreamBuffers(
        const camera3_stream_buffer_set_t * /*buffer_set*/)
{
    //Deprecated
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : processCaptureRequest
 *
 * DESCRIPTION: process a capture request from camera service
 *
 * PARAMETERS :
 *   @request : request from framework to process
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::processCaptureRequest(
                    camera3_capture_request_t *request)
{
    int rc = NO_ERROR;
    int32_t request_id;
    CameraMetadata meta;

    pthread_mutex_lock(&mMutex);

    rc = validateCaptureRequest(request);
    if (rc != NO_ERROR) {
        ALOGE("%s: incoming request is not valid", __func__);
        pthread_mutex_unlock(&mMutex);
        return rc;
    }

    meta = request->settings;

    // For first capture request, send capture intent, and
    // stream on all streams
    if (mFirstRequest) {

        for (size_t i = 0; i < request->num_output_buffers; i++) {
            const camera3_stream_buffer_t& output = request->output_buffers[i];
            QCamera3Channel *channel = (QCamera3Channel *)output.stream->priv;
            rc = channel->registerBuffer(output.buffer);
            if (rc < 0) {
                ALOGE("%s: registerBuffer failed",
                        __func__);
                pthread_mutex_unlock(&mMutex);
                return -ENODEV;
            }
        }

        if (meta.exists(ANDROID_CONTROL_CAPTURE_INTENT)) {
            int32_t hal_version = CAM_HAL_V3;
            uint8_t captureIntent =
                meta.find(ANDROID_CONTROL_CAPTURE_INTENT).data.u8[0];

            memset(mParameters, 0, sizeof(metadata_buffer_t));
            mParameters->first_flagged_entry = CAM_INTF_PARM_MAX;
            AddSetMetaEntryToBatch(mParameters, CAM_INTF_PARM_HAL_VERSION,
                sizeof(hal_version), &hal_version);
            AddSetMetaEntryToBatch(mParameters, CAM_INTF_META_CAPTURE_INTENT,
                sizeof(captureIntent), &captureIntent);
            mCameraHandle->ops->set_parms(mCameraHandle->camera_handle,
                mParameters);
        }

        //First initialize all streams
        for (List<stream_info_t *>::iterator it = mStreamInfo.begin();
            it != mStreamInfo.end(); it++) {
            QCamera3Channel *channel = (QCamera3Channel *)(*it)->stream->priv;
            rc = channel->initialize();
            if (NO_ERROR != rc) {
                ALOGE("%s : Channel initialization failed %d", __func__, rc);
                pthread_mutex_unlock(&mMutex);
                return rc;
            }
        }
        if (mSupportChannel) {
            rc = mSupportChannel->initialize();
            if (rc < 0) {
                ALOGE("%s: Support channel initialization failed", __func__);
                pthread_mutex_unlock(&mMutex);
                return rc;
            }
        }

        //Then start them.
        ALOGD("%s: Start META Channel", __func__);
        mMetadataChannel->start();

        if (mSupportChannel) {
            rc = mSupportChannel->start();
            if (rc < 0) {
                ALOGE("%s: Support channel start failed", __func__);
                mMetadataChannel->stop();
                pthread_mutex_unlock(&mMutex);
                return rc;
            }
        }
        for (List<stream_info_t *>::iterator it = mStreamInfo.begin();
            it != mStreamInfo.end(); it++) {
            QCamera3Channel *channel = (QCamera3Channel *)(*it)->stream->priv;
            ALOGD("%s: Start Regular Channel mask=%d", __func__, channel->getStreamTypeMask());
            channel->start();
        }
    }

    uint32_t frameNumber = request->frame_number;
    cam_stream_ID_t streamID;

    if (meta.exists(ANDROID_REQUEST_ID)) {
        request_id = meta.find(ANDROID_REQUEST_ID).data.i32[0];
        mCurrentRequestId = request_id;
        ALOGV("%s: Received request with id: %d",__func__, request_id);
    } else if (mFirstRequest || mCurrentRequestId == -1){
        ALOGE("%s: Unable to find request id field, \
                & no previous id available", __func__);
        return NAME_NOT_FOUND;
    } else {
        ALOGV("%s: Re-using old request id", __func__);
        request_id = mCurrentRequestId;
    }

    ALOGV("%s: %d, num_output_buffers = %d input_buffer = %p frame_number = %d",
                                    __func__, __LINE__,
                                    request->num_output_buffers,
                                    request->input_buffer,
                                    frameNumber);
    // Acquire all request buffers first
    streamID.num_streams = 0;
    int blob_request = 0;
    for (size_t i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t& output = request->output_buffers[i];
        QCamera3Channel *channel = (QCamera3Channel *)output.stream->priv;
        sp<Fence> acquireFence = new Fence(output.acquire_fence);

        if (output.stream->format == HAL_PIXEL_FORMAT_BLOB) {
            //Call function to store local copy of jpeg data for encode params.
            blob_request = 1;
        }

        rc = acquireFence->wait(Fence::TIMEOUT_NEVER);
        if (rc != OK) {
            ALOGE("%s: fence wait failed %d", __func__, rc);
            pthread_mutex_unlock(&mMutex);
            return rc;
        }

        streamID.streamID[streamID.num_streams] =
            channel->getStreamID(channel->getStreamTypeMask());
        streamID.num_streams++;
    }

    if(request->input_buffer == NULL) {
       rc = setFrameParameters(request, streamID);
        if (rc < 0) {
            ALOGE("%s: fail to set frame parameters", __func__);
            pthread_mutex_unlock(&mMutex);
            return rc;
        }
    }

    /* Update pending request list and pending buffers map */
    PendingRequestInfo pendingRequest;
    pendingRequest.frame_number = frameNumber;
    pendingRequest.num_buffers = request->num_output_buffers;
    pendingRequest.request_id = request_id;
    pendingRequest.blob_request = blob_request;
    pendingRequest.bNotified = 0;
    pendingRequest.input_buffer_present = (request->input_buffer != NULL)? 1 : 0;
    pendingRequest.pipeline_depth = 0;
    pendingRequest.partial_result_cnt = 0;
    extractJpegMetadata(pendingRequest.jpegMetadata, request);

    for (size_t i = 0; i < request->num_output_buffers; i++) {
        RequestedBufferInfo requestedBuf;
        requestedBuf.stream = request->output_buffers[i].stream;
        requestedBuf.buffer = NULL;
        pendingRequest.buffers.push_back(requestedBuf);

        // Add to buffer handle the pending buffers list
        PendingBufferInfo bufferInfo;
        bufferInfo.frame_number = frameNumber;
        bufferInfo.buffer = request->output_buffers[i].buffer;
        bufferInfo.stream = request->output_buffers[i].stream;
        mPendingBuffersMap.mPendingBufferList.push_back(bufferInfo);
        mPendingBuffersMap.num_buffers++;
        ALOGV("%s: frame = %d, buffer = %p, stream = %p, stream format = %d",
          __func__, frameNumber, bufferInfo.buffer, bufferInfo.stream,
          bufferInfo.stream->format);
    }
    ALOGV("%s: mPendingBuffersMap.num_buffers = %d",
          __func__, mPendingBuffersMap.num_buffers);

    mPendingRequestsList.push_back(pendingRequest);

    if (mFlush) {
        pthread_mutex_unlock(&mMutex);
        return NO_ERROR;
    }

    // Notify metadata channel we receive a request
    mMetadataChannel->request(NULL, frameNumber);

    // Call request on other streams
    for (size_t i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t& output = request->output_buffers[i];
        QCamera3Channel *channel = (QCamera3Channel *)output.stream->priv;
        mm_camera_buf_def_t *pInputBuffer = NULL;

        if (channel == NULL) {
            ALOGE("%s: invalid channel pointer for stream", __func__);
            continue;
        }

        if (output.stream->format == HAL_PIXEL_FORMAT_BLOB) {
            QCamera3RegularChannel* inputChannel = NULL;
            if(request->input_buffer != NULL){

                //Try to get the internal format
                inputChannel = (QCamera3RegularChannel*)
                    request->input_buffer->stream->priv;
                if(inputChannel == NULL ){
                    ALOGE("%s: failed to get input channel handle", __func__);
                } else {
                    pInputBuffer =
                        inputChannel->getInternalFormatBuffer(
                                request->input_buffer->buffer);
                    ALOGD("%s: Input buffer dump",__func__);
                    ALOGD("Stream id: %d", pInputBuffer->stream_id);
                    ALOGD("streamtype:%d", pInputBuffer->stream_type);
                    ALOGD("frame len:%d", pInputBuffer->frame_len);
                    ALOGD("Handle:%p", request->input_buffer->buffer);
                }
                rc = channel->request(output.buffer, frameNumber,
                            pInputBuffer, mParameters);
                if (rc < 0) {
                    ALOGE("%s: Fail to request on picture channel", __func__);
                    pthread_mutex_unlock(&mMutex);
                    return rc;
                }

                rc = setReprocParameters(request);
                if (rc < 0) {
                    ALOGE("%s: fail to set reproc parameters", __func__);
                    pthread_mutex_unlock(&mMutex);
                    return rc;
                }
            } else{
                 ALOGV("%s: %d, snapshot request with buffer %p, frame_number %d", __func__,
                       __LINE__, output.buffer, frameNumber);
                 if (mRepeatingRequest) {
                   rc = channel->request(output.buffer, frameNumber,
                               NULL, mPrevParameters);
                 } else {
                    rc = channel->request(output.buffer, frameNumber,
                               NULL, mParameters);
                 }
            }
        } else {
            ALOGV("%s: %d, request with buffer %p, frame_number %d", __func__,
                __LINE__, output.buffer, frameNumber);
           rc = channel->request(output.buffer, frameNumber);
        }
        if (rc < 0)
            ALOGE("%s: request failed", __func__);
    }

    /*set the parameters to backend*/
    mCameraHandle->ops->set_parms(mCameraHandle->camera_handle, mParameters);

    mFirstRequest = false;
    // Added a timed condition wait
    struct timespec ts;
    uint8_t isValidTimeout = 1;
    rc = clock_gettime(CLOCK_REALTIME, &ts);
    if (rc < 0) {
        isValidTimeout = 0;
        ALOGE("%s: Error reading the real time clock!!", __func__);
    }
    else {
        // Make timeout as 5 sec for request to be honored
        ts.tv_sec += 5;
    }
    //Block on conditional variable

    mPendingRequest++;
    while (mPendingRequest >= kMaxInFlight) {
        if (!isValidTimeout) {
            ALOGV("%s: Blocking on conditional wait", __func__);
            pthread_cond_wait(&mRequestCond, &mMutex);
        }
        else {
            ALOGV("%s: Blocking on timed conditional wait", __func__);
            rc = pthread_cond_timedwait(&mRequestCond, &mMutex, &ts);
            if (rc == ETIMEDOUT) {
                rc = -ENODEV;
                ALOGE("%s: Unblocked on timeout!!!!", __func__);
                break;
            }
        }
        ALOGV("%s: Unblocked", __func__);
    }
    pthread_mutex_unlock(&mMutex);

    return rc;
}

/*===========================================================================
 * FUNCTION   : dump
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/
void QCamera3HardwareInterface::dump(int /*fd*/)
{
    /*Enable lock when we implement this function*/
    /*
    pthread_mutex_lock(&mMutex);

    pthread_mutex_unlock(&mMutex);
    */
    return;
}

/*===========================================================================
 * FUNCTION   : flush
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/
int QCamera3HardwareInterface::flush()
{
    unsigned int frameNum = 0;
    camera3_notify_msg_t notify_msg;
    camera3_capture_result_t result;
    camera3_stream_buffer_t *pStream_Buf = NULL;
    FlushMap flushMap;

    ALOGV("%s: Unblocking Process Capture Request", __func__);

    pthread_mutex_lock(&mMutex);
    mFlush = true;
    pthread_mutex_unlock(&mMutex);

    memset(&result, 0, sizeof(camera3_capture_result_t));

    // Stop the Streams/Channels
    for (List<stream_info_t *>::iterator it = mStreamInfo.begin();
        it != mStreamInfo.end(); it++) {
        QCamera3Channel *channel = (QCamera3Channel *)(*it)->stream->priv;
        channel->stop();
        (*it)->status = INVALID;
    }

    if (mSupportChannel) {
        mSupportChannel->stop();
    }
    if (mMetadataChannel) {
        /* If content of mStreamInfo is not 0, there is metadata stream */
        mMetadataChannel->stop();
    }

    // Mutex Lock
    pthread_mutex_lock(&mMutex);

    // Unblock process_capture_request
    mPendingRequest = 0;
    pthread_cond_signal(&mRequestCond);

    List<PendingRequestInfo>::iterator i = mPendingRequestsList.begin();
    frameNum = i->frame_number;
    ALOGV("%s: Oldest frame num on  mPendingRequestsList = %d",
      __func__, frameNum);

    // Go through the pending buffers and group them depending
    // on frame number
    for (List<PendingBufferInfo>::iterator k =
            mPendingBuffersMap.mPendingBufferList.begin();
            k != mPendingBuffersMap.mPendingBufferList.end();) {

        if (k->frame_number < frameNum) {
            ssize_t idx = flushMap.indexOfKey(k->frame_number);
            if (idx == NAME_NOT_FOUND) {
                Vector<PendingBufferInfo> pending;
                pending.add(*k);
                flushMap.add(k->frame_number, pending);
            } else {
                Vector<PendingBufferInfo> &pending =
                        flushMap.editValueFor(k->frame_number);
                pending.add(*k);
            }

            mPendingBuffersMap.num_buffers--;
            k = mPendingBuffersMap.mPendingBufferList.erase(k);
        } else {
            k++;
        }
    }

    for (size_t i = 0; i < flushMap.size(); i++) {
        uint32_t frame_number = flushMap.keyAt(i);
        const Vector<PendingBufferInfo> &pending = flushMap.valueAt(i);

        // Send Error notify to frameworks for each buffer for which
        // metadata buffer is already sent
        ALOGV("%s: Sending ERROR BUFFER for frame %d number of buffer %d",
          __func__, frame_number, pending.size());

        pStream_Buf = new camera3_stream_buffer_t[pending.size()];
        if (NULL == pStream_Buf) {
            ALOGE("%s: No memory for pending buffers array", __func__);
            pthread_mutex_unlock(&mMutex);
            return NO_MEMORY;
        }

        for (size_t j = 0; j < pending.size(); j++) {
            const PendingBufferInfo &info = pending.itemAt(j);
            notify_msg.type = CAMERA3_MSG_ERROR;
            notify_msg.message.error.error_code = CAMERA3_MSG_ERROR_BUFFER;
            notify_msg.message.error.error_stream = info.stream;
            notify_msg.message.error.frame_number = frame_number;
            pStream_Buf[j].acquire_fence = -1;
            pStream_Buf[j].release_fence = -1;
            pStream_Buf[j].buffer = info.buffer;
            pStream_Buf[j].status = CAMERA3_BUFFER_STATUS_ERROR;
            pStream_Buf[j].stream = info.stream;
            mCallbackOps->notify(mCallbackOps, &notify_msg);
            ALOGV("%s: notify frame_number = %d stream %p", __func__,
                    frame_number, info.stream);
        }

        result.result = NULL;
        result.frame_number = frame_number;
        result.num_output_buffers = pending.size();
        result.output_buffers = pStream_Buf;
        mCallbackOps->process_capture_result(mCallbackOps, &result);

        delete [] pStream_Buf;
    }

    ALOGV("%s:Sending ERROR REQUEST for all pending requests", __func__);

    flushMap.clear();
    for (List<PendingBufferInfo>::iterator k =
            mPendingBuffersMap.mPendingBufferList.begin();
            k != mPendingBuffersMap.mPendingBufferList.end();) {
        ssize_t idx = flushMap.indexOfKey(k->frame_number);
        if (idx == NAME_NOT_FOUND) {
            Vector<PendingBufferInfo> pending;
            pending.add(*k);
            flushMap.add(k->frame_number, pending);
        } else {
            Vector<PendingBufferInfo> &pending =
                    flushMap.editValueFor(k->frame_number);
            pending.add(*k);
        }

        mPendingBuffersMap.num_buffers--;
        k = mPendingBuffersMap.mPendingBufferList.erase(k);
    }

    // Go through the pending requests info and send error request to framework
    for (size_t i = 0; i < flushMap.size(); i++) {
        uint32_t frame_number = flushMap.keyAt(i);
        const Vector<PendingBufferInfo> &pending = flushMap.valueAt(i);
        ALOGV("%s:Sending ERROR REQUEST for frame %d",
              __func__, frame_number);

        // Send shutter notify to frameworks
        notify_msg.type = CAMERA3_MSG_ERROR;
        notify_msg.message.error.error_code = CAMERA3_MSG_ERROR_REQUEST;
        notify_msg.message.error.error_stream = NULL;
        notify_msg.message.error.frame_number = frame_number;
        mCallbackOps->notify(mCallbackOps, &notify_msg);

        pStream_Buf = new camera3_stream_buffer_t[pending.size()];
        if (NULL == pStream_Buf) {
            ALOGE("%s: No memory for pending buffers array", __func__);
            pthread_mutex_unlock(&mMutex);
            return NO_MEMORY;
        }

        for (size_t j = 0; j < pending.size(); j++) {
            const PendingBufferInfo &info = pending.itemAt(j);
            pStream_Buf[j].acquire_fence = -1;
            pStream_Buf[j].release_fence = -1;
            pStream_Buf[j].buffer = info.buffer;
            pStream_Buf[j].status = CAMERA3_BUFFER_STATUS_ERROR;
            pStream_Buf[j].stream = info.stream;
        }

        result.num_output_buffers = pending.size();
        result.output_buffers = pStream_Buf;
        result.result = NULL;
        result.frame_number = frame_number;
        mCallbackOps->process_capture_result(mCallbackOps, &result);
        delete [] pStream_Buf;
    }

    /* Reset pending buffer list and requests list */
    mPendingRequestsList.clear();
    /* Reset pending frame Drop list and requests list */
    mPendingFrameDropList.clear();

    flushMap.clear();
    mPendingBuffersMap.num_buffers = 0;
    mPendingBuffersMap.mPendingBufferList.clear();
    ALOGV("%s: Cleared all the pending buffers ", __func__);

    mFlush = false;

    mFirstRequest = true;

    // Start the Streams/Channels
    if (mMetadataChannel) {
        /* If content of mStreamInfo is not 0, there is metadata stream */
        mMetadataChannel->start();
    }
    for (List<stream_info_t *>::iterator it = mStreamInfo.begin();
        it != mStreamInfo.end(); it++) {
        QCamera3Channel *channel = (QCamera3Channel *)(*it)->stream->priv;
        channel->start();
    }
    if (mSupportChannel) {
        mSupportChannel->start();
    }

    pthread_mutex_unlock(&mMutex);
    return 0;
}

/*===========================================================================
 * FUNCTION   : captureResultCb
 *
 * DESCRIPTION: Callback handler for all capture result
 *              (streams, as well as metadata)
 *
 * PARAMETERS :
 *   @metadata : metadata information
 *   @buffer   : actual gralloc buffer to be returned to frameworks.
 *               NULL if metadata.
 *
 * RETURN     : NONE
 *==========================================================================*/
void QCamera3HardwareInterface::captureResultCb(mm_camera_super_buf_t *metadata_buf,
                camera3_stream_buffer_t *buffer, uint32_t frame_number)
{
    pthread_mutex_lock(&mMutex);

    /* Assume flush() is called before any reprocessing. Send
     * notify and result immediately upon receipt of any callback*/
    if (mLoopBackResult) {
        /* Send notify */
        camera3_notify_msg_t notify_msg;
        notify_msg.type = CAMERA3_MSG_SHUTTER;
        notify_msg.message.shutter.frame_number = mLoopBackResult->frame_number;
        notify_msg.message.shutter.timestamp = mLoopBackTimestamp;
        mCallbackOps->notify(mCallbackOps, &notify_msg);

        /* Send capture result */
        mCallbackOps->process_capture_result(mCallbackOps, mLoopBackResult);
        free_camera_metadata((camera_metadata_t *)mLoopBackResult->result);
        free(mLoopBackResult);
        mLoopBackResult = NULL;
    }

    if (metadata_buf)
        handleMetadataWithLock(metadata_buf);
    else
        handleBufferWithLock(buffer, frame_number);

    pthread_mutex_unlock(&mMutex);
    return;
}

/*===========================================================================
 * FUNCTION   : translateFromHalMetadata
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *   @metadata : metadata information from callback
 *
 * RETURN     : camera_metadata_t*
 *              metadata in a format specified by fwk
 *==========================================================================*/
camera_metadata_t*
QCamera3HardwareInterface::translateFromHalMetadata(
                                 metadata_buffer_t *metadata,
                                 nsecs_t timestamp,
                                 int32_t request_id,
                                 const CameraMetadata& jpegMetadata,
                                 uint8_t pipeline_depth)
{
    CameraMetadata camMetadata;
    camera_metadata_t* resultMetadata;

    if (jpegMetadata.entryCount())
        camMetadata.append(jpegMetadata);

    camMetadata.update(ANDROID_SENSOR_TIMESTAMP, &timestamp, 1);
    camMetadata.update(ANDROID_REQUEST_ID, &request_id, 1);
    camMetadata.update(ANDROID_REQUEST_PIPELINE_DEPTH, &pipeline_depth, 1);

    uint8_t curr_entry = GET_FIRST_PARAM_ID(metadata);
    uint8_t next_entry;
    while (curr_entry != CAM_INTF_PARM_MAX) {
       switch (curr_entry) {
         case CAM_INTF_META_FRAME_NUMBER:{
             int64_t frame_number = *(uint32_t *) POINTER_OF(CAM_INTF_META_FRAME_NUMBER, metadata);
             camMetadata.update(ANDROID_SYNC_FRAME_NUMBER, &frame_number, 1);
             break;
         }
         case CAM_INTF_META_FACE_DETECTION:{
             cam_face_detection_data_t *faceDetectionInfo =
                (cam_face_detection_data_t *)POINTER_OF(CAM_INTF_META_FACE_DETECTION, metadata);
             uint8_t numFaces = faceDetectionInfo->num_faces_detected;
             int32_t faceIds[MAX_ROI];
             uint8_t faceScores[MAX_ROI];
             int32_t faceRectangles[MAX_ROI * 4];
             int j = 0;
             for (int i = 0; i < numFaces; i++) {
                 faceIds[i] = faceDetectionInfo->faces[i].face_id;
                 faceScores[i] = faceDetectionInfo->faces[i].score;
                 convertToRegions(faceDetectionInfo->faces[i].face_boundary,
                         faceRectangles+j, -1);
                 j+= 4;
             }

             if (numFaces <= 0) {
                memset(faceIds, 0, sizeof(int32_t) * MAX_ROI);
                memset(faceScores, 0, sizeof(uint8_t) * MAX_ROI);
                memset(faceRectangles, 0, sizeof(int32_t) * MAX_ROI * 4);
             }

             camMetadata.update(ANDROID_STATISTICS_FACE_IDS, faceIds, numFaces);
             camMetadata.update(ANDROID_STATISTICS_FACE_SCORES, faceScores, numFaces);
             camMetadata.update(ANDROID_STATISTICS_FACE_RECTANGLES,
               faceRectangles, numFaces*4);
            break;
            }
         case CAM_INTF_META_COLOR_CORRECT_MODE:{
             uint8_t  *color_correct_mode =
                           (uint8_t *)POINTER_OF(CAM_INTF_META_COLOR_CORRECT_MODE, metadata);
             camMetadata.update(ANDROID_COLOR_CORRECTION_MODE, color_correct_mode, 1);
             break;
          }

         // 3A state is sent in urgent partial result (uses quirk)
         case CAM_INTF_META_AEC_STATE:
         case CAM_INTF_PARM_AEC_LOCK:
         case CAM_INTF_PARM_EV:
         case CAM_INTF_PARM_FOCUS_MODE:
         case CAM_INTF_META_AF_STATE:
         case CAM_INTF_PARM_WHITE_BALANCE:
         case CAM_INTF_META_AWB_REGIONS:
         case CAM_INTF_META_AWB_STATE:
         case CAM_INTF_PARM_AWB_LOCK:
         case CAM_INTF_META_PRECAPTURE_TRIGGER:
         case CAM_INTF_META_AEC_MODE:
         case CAM_INTF_PARM_LED_MODE:
         case CAM_INTF_PARM_REDEYE_REDUCTION:
         case CAM_INTF_META_AF_TRIGGER_NOTICE: {
           ALOGV("%s: 3A metadata: %d, do not process", __func__, curr_entry);
           break;
         }

          case CAM_INTF_META_MODE: {
             uint8_t *mode =(uint8_t *)POINTER_OF(CAM_INTF_META_MODE, metadata);
             camMetadata.update(ANDROID_CONTROL_MODE, mode, 1);
             break;
          }

          case CAM_INTF_META_EDGE_MODE: {
             cam_edge_application_t  *edgeApplication =
                (cam_edge_application_t *)POINTER_OF(CAM_INTF_META_EDGE_MODE, metadata);
             uint8_t edgeStrength = (uint8_t)edgeApplication->sharpness;
             camMetadata.update(ANDROID_EDGE_MODE, &(edgeApplication->edge_mode), 1);
             camMetadata.update(ANDROID_EDGE_STRENGTH, &edgeStrength, 1);
             break;
          }
          case CAM_INTF_META_FLASH_POWER: {
             uint8_t  *flashPower =
                  (uint8_t *)POINTER_OF(CAM_INTF_META_FLASH_POWER, metadata);
             camMetadata.update(ANDROID_FLASH_FIRING_POWER, flashPower, 1);
             break;
          }
          case CAM_INTF_META_FLASH_FIRING_TIME: {
             int64_t  *flashFiringTime =
                  (int64_t *)POINTER_OF(CAM_INTF_META_FLASH_FIRING_TIME, metadata);
             camMetadata.update(ANDROID_FLASH_FIRING_TIME, flashFiringTime, 1);
             break;
          }
          case CAM_INTF_META_FLASH_STATE: {
             uint8_t  flashState =
                *((uint8_t *)POINTER_OF(CAM_INTF_META_FLASH_STATE, metadata));
             if (!gCamCapability[mCameraId]->flash_available) {
                 flashState = ANDROID_FLASH_STATE_UNAVAILABLE;
             }
             camMetadata.update(ANDROID_FLASH_STATE, &flashState, 1);
             break;
          }
          case CAM_INTF_META_FLASH_MODE:{
             uint8_t flashMode = *((uint8_t*)
                 POINTER_OF(CAM_INTF_META_FLASH_MODE, metadata));
             uint8_t fwk_flashMode = lookupFwkName(FLASH_MODES_MAP,
                                          sizeof(FLASH_MODES_MAP),
                                          flashMode);
             camMetadata.update(ANDROID_FLASH_MODE, &fwk_flashMode, 1);
             break;
          }
          case CAM_INTF_META_HOTPIXEL_MODE: {
              uint8_t  *hotPixelMode =
                 (uint8_t *)POINTER_OF(CAM_INTF_META_HOTPIXEL_MODE, metadata);
              camMetadata.update(ANDROID_HOT_PIXEL_MODE, hotPixelMode, 1);
              break;
          }
          case CAM_INTF_META_LENS_APERTURE:{
             float  *lensAperture =
                (float *)POINTER_OF(CAM_INTF_META_LENS_APERTURE, metadata);
             camMetadata.update(ANDROID_LENS_APERTURE , lensAperture, 1);
             break;
          }
          case CAM_INTF_META_LENS_FILTERDENSITY: {
             float  *filterDensity =
                (float *)POINTER_OF(CAM_INTF_META_LENS_FILTERDENSITY, metadata);
             camMetadata.update(ANDROID_LENS_FILTER_DENSITY , filterDensity, 1);
             break;
          }
          case CAM_INTF_META_LENS_FOCAL_LENGTH:{
             float  *focalLength =
                (float *)POINTER_OF(CAM_INTF_META_LENS_FOCAL_LENGTH, metadata);
             camMetadata.update(ANDROID_LENS_FOCAL_LENGTH, focalLength, 1);
             break;
          }
          case CAM_INTF_META_LENS_FOCUS_DISTANCE: {
             float  *focusDistance =
                (float *)POINTER_OF(CAM_INTF_META_LENS_FOCUS_DISTANCE, metadata);
             camMetadata.update(ANDROID_LENS_FOCUS_DISTANCE , focusDistance, 1);
             break;
          }
          case CAM_INTF_META_LENS_FOCUS_RANGE: {
             float  *focusRange =
                (float *)POINTER_OF(CAM_INTF_META_LENS_FOCUS_RANGE, metadata);
             camMetadata.update(ANDROID_LENS_FOCUS_RANGE , focusRange, 2);
             break;
          }
          case CAM_INTF_META_LENS_STATE: {
             uint8_t *lensState = (uint8_t *)POINTER_OF(CAM_INTF_META_LENS_STATE, metadata);
             camMetadata.update(ANDROID_LENS_STATE , lensState, 1);
             break;
          }
          case CAM_INTF_META_LENS_OPT_STAB_MODE: {
             uint8_t  *opticalStab =
                (uint8_t *)POINTER_OF(CAM_INTF_META_LENS_OPT_STAB_MODE, metadata);
             camMetadata.update(ANDROID_LENS_OPTICAL_STABILIZATION_MODE ,opticalStab, 1);
             break;
          }
          case CAM_INTF_META_NOISE_REDUCTION_MODE: {
             uint8_t  *noiseRedMode =
                (uint8_t *)POINTER_OF(CAM_INTF_META_NOISE_REDUCTION_MODE, metadata);
             camMetadata.update(ANDROID_NOISE_REDUCTION_MODE , noiseRedMode, 1);
             break;
          }
          case CAM_INTF_META_NOISE_REDUCTION_STRENGTH: {
             uint8_t  *noiseRedStrength =
                (uint8_t *)POINTER_OF(CAM_INTF_META_NOISE_REDUCTION_STRENGTH, metadata);
             camMetadata.update(ANDROID_NOISE_REDUCTION_STRENGTH, noiseRedStrength, 1);
             break;
          }
          case CAM_INTF_META_SCALER_CROP_REGION: {
             cam_crop_region_t  *hScalerCropRegion =(cam_crop_region_t *)
             POINTER_OF(CAM_INTF_META_SCALER_CROP_REGION, metadata);
             int32_t scalerCropRegion[4];
             scalerCropRegion[0] = hScalerCropRegion->left;
             scalerCropRegion[1] = hScalerCropRegion->top;
             scalerCropRegion[2] = hScalerCropRegion->width;
             scalerCropRegion[3] = hScalerCropRegion->height;
             camMetadata.update(ANDROID_SCALER_CROP_REGION, scalerCropRegion, 4);
             break;
          }
          case CAM_INTF_META_AEC_ROI: {
            cam_area_t  *hAeRegions =
                (cam_area_t *)POINTER_OF(CAM_INTF_META_AEC_ROI, metadata);
            int32_t aeRegions[5];
            convertToRegions(hAeRegions->rect, aeRegions, hAeRegions->weight);
            camMetadata.update(ANDROID_CONTROL_AE_REGIONS, aeRegions, 5);
            ALOGV("%s: Metadata : ANDROID_CONTROL_AE_REGIONS: FWK: [%d, %d, %d, %d] HAL: [%d, %d, %d, %d]",
                __func__, aeRegions[0], aeRegions[1], aeRegions[2], aeRegions[3],
                hAeRegions->rect.left, hAeRegions->rect.top, hAeRegions->rect.width, hAeRegions->rect.height);
            break;
          }
          case CAM_INTF_META_AF_ROI:{
            /*af regions*/
            cam_area_t  *hAfRegions =
                (cam_area_t *)POINTER_OF(CAM_INTF_META_AF_ROI, metadata);
            int32_t afRegions[5];
            convertToRegions(hAfRegions->rect, afRegions, hAfRegions->weight);
            camMetadata.update(ANDROID_CONTROL_AF_REGIONS, afRegions, 5);
            ALOGV("%s: Metadata : ANDROID_CONTROL_AF_REGIONS: FWK: [%d, %d, %d, %d] HAL: [%d, %d, %d, %d]",
                __func__, afRegions[0], afRegions[1], afRegions[2], afRegions[3],
                hAfRegions->rect.left, hAfRegions->rect.top, hAfRegions->rect.width, hAfRegions->rect.height);
            break;
          }
          case CAM_INTF_META_SENSOR_EXPOSURE_TIME:{
             int64_t  *sensorExpTime =
                (int64_t *)POINTER_OF(CAM_INTF_META_SENSOR_EXPOSURE_TIME, metadata);
             ALOGV("%s: sensorExpTime = %lld", __func__, *sensorExpTime);
             camMetadata.update(ANDROID_SENSOR_EXPOSURE_TIME , sensorExpTime, 1);
             break;
          }
          case CAM_INTF_META_SENSOR_ROLLING_SHUTTER_SKEW:{
             int64_t  *sensorRollingShutterSkew =
                (int64_t *)POINTER_OF(CAM_INTF_META_SENSOR_ROLLING_SHUTTER_SKEW,
                  metadata);
             ALOGV("%s: sensorRollingShutterSkew = %lld", __func__,
               *sensorRollingShutterSkew);
             camMetadata.update(ANDROID_SENSOR_ROLLING_SHUTTER_SKEW ,
               sensorRollingShutterSkew, 1);
             break;
          }
          case CAM_INTF_META_SENSOR_FRAME_DURATION:{
             int64_t  *sensorFameDuration =
                (int64_t *)POINTER_OF(CAM_INTF_META_SENSOR_FRAME_DURATION, metadata);
             ALOGV("%s: sensorFameDuration = %lld", __func__, *sensorFameDuration);
             camMetadata.update(ANDROID_SENSOR_FRAME_DURATION, sensorFameDuration, 1);
             break;
          }
          case CAM_INTF_META_SENSOR_SENSITIVITY:{
            int32_t sensorSensitivity =
               *((int32_t *)POINTER_OF(CAM_INTF_META_SENSOR_SENSITIVITY, metadata));
            ALOGV("%s: sensorSensitivity = %d", __func__, sensorSensitivity);
            camMetadata.update(ANDROID_SENSOR_SENSITIVITY, &sensorSensitivity, 1);

            double noise_profile_S = computeNoiseModelEntryS(sensorSensitivity);
            double noise_profile_O = computeNoiseModelEntryO(sensorSensitivity);
            double noise_profile[2 * gCamCapability[mCameraId]->num_color_channels];
            for(int i = 0; i < 2 * gCamCapability[mCameraId]->num_color_channels; i+=2){
                noise_profile[i]   = noise_profile_S;
                noise_profile[i+1] = noise_profile_O;
            }
            camMetadata.update(ANDROID_SENSOR_NOISE_PROFILE, noise_profile,
                2 * gCamCapability[mCameraId]->num_color_channels);
            break;
          }
          case CAM_INTF_PARM_BESTSHOT_MODE: {
              uint8_t *sceneMode =
                  (uint8_t *)POINTER_OF(CAM_INTF_PARM_BESTSHOT_MODE, metadata);
              uint8_t fwkSceneMode =
                  (uint8_t)lookupFwkName(SCENE_MODES_MAP,
                  sizeof(SCENE_MODES_MAP)/
                  sizeof(SCENE_MODES_MAP[0]), *sceneMode);
              camMetadata.update(ANDROID_CONTROL_SCENE_MODE,
                   &fwkSceneMode, 1);
              ALOGV("%s: Metadata : ANDROID_CONTROL_SCENE_MODE: %d", __func__, fwkSceneMode);
              break;
          }

          case CAM_INTF_META_SHADING_MODE: {
             uint8_t  *shadingMode =
                (uint8_t *)POINTER_OF(CAM_INTF_META_SHADING_MODE, metadata);
             camMetadata.update(ANDROID_SHADING_MODE, shadingMode, 1);
             break;
          }

          case CAM_INTF_META_LENS_SHADING_MAP_MODE: {
             uint8_t  *shadingMapMode =
                (uint8_t *)POINTER_OF(CAM_INTF_META_LENS_SHADING_MAP_MODE, metadata);
             camMetadata.update(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, shadingMapMode, 1);
             break;
          }

          case CAM_INTF_META_STATS_FACEDETECT_MODE: {
             uint8_t  *faceDetectMode =
                (uint8_t *)POINTER_OF(CAM_INTF_META_STATS_FACEDETECT_MODE, metadata);
             uint8_t fwk_faceDetectMode = (uint8_t)lookupFwkName(FACEDETECT_MODES_MAP,
                                                        sizeof(FACEDETECT_MODES_MAP)/sizeof(FACEDETECT_MODES_MAP[0]),
                                                        *faceDetectMode);
             /* Downgrade to simple mode */
             if (fwk_faceDetectMode == ANDROID_STATISTICS_FACE_DETECT_MODE_FULL) {
                 fwk_faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE;
             }
             camMetadata.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &fwk_faceDetectMode, 1);
             break;
          }
          case CAM_INTF_META_STATS_HISTOGRAM_MODE: {
             uint8_t  *histogramMode =
                (uint8_t *)POINTER_OF(CAM_INTF_META_STATS_HISTOGRAM_MODE, metadata);
             camMetadata.update(ANDROID_STATISTICS_HISTOGRAM_MODE, histogramMode, 1);
             break;
          }
          case CAM_INTF_META_STATS_SHARPNESS_MAP_MODE:{
               uint8_t  *sharpnessMapMode =
                  (uint8_t *)POINTER_OF(CAM_INTF_META_STATS_SHARPNESS_MAP_MODE, metadata);
               camMetadata.update(ANDROID_STATISTICS_SHARPNESS_MAP_MODE,
                                  sharpnessMapMode, 1);
               break;
           }
          case CAM_INTF_META_STATS_SHARPNESS_MAP:{
               cam_sharpness_map_t  *sharpnessMap = (cam_sharpness_map_t *)
               POINTER_OF(CAM_INTF_META_STATS_SHARPNESS_MAP, metadata);
               camMetadata.update(ANDROID_STATISTICS_SHARPNESS_MAP,
                                  (int32_t*)sharpnessMap->sharpness,
                                  CAM_MAX_MAP_WIDTH*CAM_MAX_MAP_HEIGHT);
               break;
          }
          case CAM_INTF_META_LENS_SHADING_MAP: {
               cam_lens_shading_map_t *lensShadingMap = (cam_lens_shading_map_t *)
               POINTER_OF(CAM_INTF_META_LENS_SHADING_MAP, metadata);
               int map_height = gCamCapability[mCameraId]->lens_shading_map_size.height;
               int map_width  = gCamCapability[mCameraId]->lens_shading_map_size.width;
               camMetadata.update(ANDROID_STATISTICS_LENS_SHADING_MAP,
                                  (float*)lensShadingMap->lens_shading,
                                  4*map_width*map_height);
               break;
          }

          case CAM_INTF_META_TONEMAP_MODE: {
             uint8_t  *toneMapMode =
                (uint8_t *)POINTER_OF(CAM_INTF_META_TONEMAP_MODE, metadata);
             camMetadata.update(ANDROID_TONEMAP_MODE, toneMapMode, 1);
             break;
          }

          case CAM_INTF_META_TONEMAP_CURVES:{
             //Populate CAM_INTF_META_TONEMAP_CURVES
             /* ch0 = G, ch 1 = B, ch 2 = R*/
             cam_rgb_tonemap_curves *tonemap = (cam_rgb_tonemap_curves *)
             POINTER_OF(CAM_INTF_META_TONEMAP_CURVES, metadata);
             camMetadata.update(ANDROID_TONEMAP_CURVE_GREEN,
                                (float*)tonemap->curves[0].tonemap_points,
                                tonemap->tonemap_points_cnt * 2);

             camMetadata.update(ANDROID_TONEMAP_CURVE_BLUE,
                                (float*)tonemap->curves[1].tonemap_points,
                                tonemap->tonemap_points_cnt * 2);

             camMetadata.update(ANDROID_TONEMAP_CURVE_RED,
                                (float*)tonemap->curves[2].tonemap_points,
                                tonemap->tonemap_points_cnt * 2);
             break;
          }

          case CAM_INTF_META_COLOR_CORRECT_GAINS:{
             cam_color_correct_gains_t *colorCorrectionGains = (cam_color_correct_gains_t*)
             POINTER_OF(CAM_INTF_META_COLOR_CORRECT_GAINS, metadata);
             camMetadata.update(ANDROID_COLOR_CORRECTION_GAINS, colorCorrectionGains->gains, 4);
             break;
          }
          case CAM_INTF_META_COLOR_CORRECT_TRANSFORM:{
              cam_color_correct_matrix_t *colorCorrectionMatrix = (cam_color_correct_matrix_t*)
              POINTER_OF(CAM_INTF_META_COLOR_CORRECT_TRANSFORM, metadata);
              camMetadata.update(ANDROID_COLOR_CORRECTION_TRANSFORM,
                       (camera_metadata_rational_t*)colorCorrectionMatrix->transform_matrix, 3*3);
              break;
          }

          /* DNG file realted metadata */
          case CAM_INTF_META_PROFILE_TONE_CURVE: {
             cam_profile_tone_curve *toneCurve = (cam_profile_tone_curve *)
             POINTER_OF(CAM_INTF_META_PROFILE_TONE_CURVE, metadata);
             camMetadata.update(ANDROID_SENSOR_PROFILE_TONE_CURVE,
                                (float*)toneCurve->curve.tonemap_points,
                                toneCurve->tonemap_points_cnt * 2);
             break;
          }

          case CAM_INTF_META_PRED_COLOR_CORRECT_GAINS:{
             cam_color_correct_gains_t *predColorCorrectionGains = (cam_color_correct_gains_t*)
             POINTER_OF(CAM_INTF_META_PRED_COLOR_CORRECT_GAINS, metadata);
             camMetadata.update(ANDROID_STATISTICS_PREDICTED_COLOR_GAINS,
                       predColorCorrectionGains->gains, 4);
             break;
          }
          case CAM_INTF_META_PRED_COLOR_CORRECT_TRANSFORM:{
             cam_color_correct_matrix_t *predColorCorrectionMatrix = (cam_color_correct_matrix_t*)
                   POINTER_OF(CAM_INTF_META_PRED_COLOR_CORRECT_TRANSFORM, metadata);
             camMetadata.update(ANDROID_STATISTICS_PREDICTED_COLOR_TRANSFORM,
                                  (camera_metadata_rational_t*)predColorCorrectionMatrix->transform_matrix, 3*3);
             break;

          }

          case CAM_INTF_META_OTP_WB_GRGB:{
             float *otpWbGrGb = (float*) POINTER_OF(CAM_INTF_META_OTP_WB_GRGB, metadata);
             camMetadata.update(ANDROID_SENSOR_GREEN_SPLIT, otpWbGrGb, 1);
             break;
          }

          case CAM_INTF_META_BLACK_LEVEL_LOCK:{
             uint8_t *blackLevelLock = (uint8_t*)
               POINTER_OF(CAM_INTF_META_BLACK_LEVEL_LOCK, metadata);
             camMetadata.update(ANDROID_BLACK_LEVEL_LOCK, blackLevelLock, 1);
             break;
          }
          case CAM_INTF_PARM_ANTIBANDING: {
            uint8_t *hal_ab_mode =
              (uint8_t *)POINTER_OF(CAM_INTF_PARM_ANTIBANDING, metadata);
            uint8_t fwk_ab_mode = (uint8_t)lookupFwkName(ANTIBANDING_MODES_MAP,
                     sizeof(ANTIBANDING_MODES_MAP)/sizeof(ANTIBANDING_MODES_MAP[0]),
                     *hal_ab_mode);
            camMetadata.update(ANDROID_CONTROL_AE_ANTIBANDING_MODE,
                &fwk_ab_mode, 1);
            break;
          }

          case CAM_INTF_META_CAPTURE_INTENT:{
             uint8_t *captureIntent = (uint8_t*)
               POINTER_OF(CAM_INTF_META_CAPTURE_INTENT, metadata);
             camMetadata.update(ANDROID_CONTROL_CAPTURE_INTENT, captureIntent, 1);
             break;
          }

          case CAM_INTF_META_SCENE_FLICKER:{
             uint8_t *sceneFlicker = (uint8_t*)
             POINTER_OF(CAM_INTF_META_SCENE_FLICKER, metadata);
             camMetadata.update(ANDROID_STATISTICS_SCENE_FLICKER, sceneFlicker, 1);
             break;
          }
          case CAM_INTF_PARM_EFFECT: {
             uint8_t *effectMode = (uint8_t*)
                  POINTER_OF(CAM_INTF_PARM_EFFECT, metadata);
             uint8_t fwk_effectMode = (uint8_t)lookupFwkName(EFFECT_MODES_MAP,
                                                    sizeof(EFFECT_MODES_MAP),
                                                    *effectMode);
             camMetadata.update(ANDROID_CONTROL_EFFECT_MODE, &fwk_effectMode, 1);
             break;
          }
          case CAM_INTF_META_TEST_PATTERN_DATA: {
             cam_test_pattern_data_t *testPatternData = (cam_test_pattern_data_t *)
                 POINTER_OF(CAM_INTF_META_TEST_PATTERN_DATA, metadata);
             int32_t fwk_testPatternMode = lookupFwkName(TEST_PATTERN_MAP,
                     sizeof(TEST_PATTERN_MAP)/sizeof(TEST_PATTERN_MAP[0]),
                     testPatternData->mode);
             camMetadata.update(ANDROID_SENSOR_TEST_PATTERN_MODE,
                     &fwk_testPatternMode, 1);
            int32_t fwk_testPatternData[4];
            fwk_testPatternData[0] = testPatternData->r;
            fwk_testPatternData[3] = testPatternData->b;
            switch (gCamCapability[mCameraId]->color_arrangement) {
            case CAM_FILTER_ARRANGEMENT_RGGB:
            case CAM_FILTER_ARRANGEMENT_GRBG:
                fwk_testPatternData[1] = testPatternData->gr;
                fwk_testPatternData[2] = testPatternData->gb;
                break;
            case CAM_FILTER_ARRANGEMENT_GBRG:
            case CAM_FILTER_ARRANGEMENT_BGGR:
                fwk_testPatternData[2] = testPatternData->gr;
                fwk_testPatternData[1] = testPatternData->gb;
                break;
            default:
                ALOGE("%s: color arrangement %d is not supported", __func__,
                    gCamCapability[mCameraId]->color_arrangement);
                break;
            }
            camMetadata.update(ANDROID_SENSOR_TEST_PATTERN_DATA, fwk_testPatternData, 4);
            break;

          }
          case CAM_INTF_META_JPEG_GPS_COORDINATES: {
              double *gps_coords = (double *)POINTER_OF(
                      CAM_INTF_META_JPEG_GPS_COORDINATES, metadata);
              camMetadata.update(ANDROID_JPEG_GPS_COORDINATES, gps_coords, 3);
              break;
          }
          case CAM_INTF_META_JPEG_GPS_PROC_METHODS: {
              char *gps_methods = (char *)POINTER_OF(
                      CAM_INTF_META_JPEG_GPS_PROC_METHODS, metadata);
              String8 str(gps_methods);
              camMetadata.update(ANDROID_JPEG_GPS_PROCESSING_METHOD, str);
              break;
          }
          case CAM_INTF_META_JPEG_GPS_TIMESTAMP: {
              int64_t *gps_timestamp = (int64_t *)POINTER_OF(
                      CAM_INTF_META_JPEG_GPS_TIMESTAMP, metadata);
              camMetadata.update(ANDROID_JPEG_GPS_TIMESTAMP, gps_timestamp, 1);
              break;
          }
          case CAM_INTF_META_JPEG_ORIENTATION: {
              int32_t *jpeg_orientation = (int32_t *)POINTER_OF(
                      CAM_INTF_META_JPEG_ORIENTATION, metadata);
              camMetadata.update(ANDROID_JPEG_ORIENTATION, jpeg_orientation, 1);
              break;
          }
          case CAM_INTF_META_JPEG_QUALITY: {
              uint8_t *jpeg_quality = (uint8_t *)POINTER_OF(
                      CAM_INTF_META_JPEG_QUALITY, metadata);
              camMetadata.update(ANDROID_JPEG_QUALITY, jpeg_quality, 1);
              break;
          }
          case CAM_INTF_META_JPEG_THUMB_QUALITY: {
              uint8_t *thumb_quality = (uint8_t *)POINTER_OF(
                      CAM_INTF_META_JPEG_THUMB_QUALITY, metadata);
              camMetadata.update(ANDROID_JPEG_THUMBNAIL_QUALITY, thumb_quality, 1);
              break;
          }

          case CAM_INTF_META_JPEG_THUMB_SIZE: {
              cam_dimension_t *thumb_size = (cam_dimension_t *)POINTER_OF(
                      CAM_INTF_META_JPEG_THUMB_SIZE, metadata);
              camMetadata.update(ANDROID_JPEG_THUMBNAIL_SIZE, (int32_t *)thumb_size, 2);
              break;
          }

             break;
          case CAM_INTF_META_PRIVATE_DATA: {
             uint8_t *privateData = (uint8_t *)
                 POINTER_OF(CAM_INTF_META_PRIVATE_DATA, metadata);
             camMetadata.update(QCAMERA3_PRIVATEDATA_REPROCESS,
                 privateData, MAX_METADATA_PAYLOAD_SIZE);
             break;
          }

          case CAM_INTF_META_NEUTRAL_COL_POINT:{
             cam_neutral_col_point_t *neuColPoint = (cam_neutral_col_point_t*)
                 POINTER_OF(CAM_INTF_META_NEUTRAL_COL_POINT, metadata);
             camMetadata.update(ANDROID_SENSOR_NEUTRAL_COLOR_POINT,
                     (camera_metadata_rational_t*)neuColPoint->neutral_col_point, 3);
             break;
          }

          default:
             ALOGV("%s: This is not a valid metadata type to report to fwk, %d",
                   __func__, curr_entry);
             break;
       }
       next_entry = GET_NEXT_PARAM_ID(curr_entry, metadata);
       curr_entry = next_entry;
    }

    /* Constant metadata values to be update*/
    uint8_t vs_mode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
    camMetadata.update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &vs_mode, 1);

    uint8_t hotPixelMode = ANDROID_HOT_PIXEL_MODE_FAST;
    camMetadata.update(ANDROID_HOT_PIXEL_MODE, &hotPixelMode, 1);

    uint8_t hotPixelMapMode = ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_OFF;
    camMetadata.update(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE, &hotPixelMapMode, 1);

    int32_t hotPixelMap[2];
    camMetadata.update(ANDROID_STATISTICS_HOT_PIXEL_MAP, &hotPixelMap[0], 0);

    uint8_t cac = ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF;
    camMetadata.update(ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
                       &cac,
                       1);

    resultMetadata = camMetadata.release();
    return resultMetadata;
}

/*===========================================================================
 * FUNCTION   : translateCbUrgentMetadataToResultMetadata
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *   @metadata : metadata information from callback
 *
 * RETURN     : camera_metadata_t*
 *              metadata in a format specified by fwk
 *==========================================================================*/
camera_metadata_t*
QCamera3HardwareInterface::translateCbUrgentMetadataToResultMetadata
                                (metadata_buffer_t *metadata)
{
    CameraMetadata camMetadata;
    camera_metadata_t* resultMetadata;
    uint8_t *aeMode = NULL;
    int32_t *flashMode = NULL;
    int32_t *redeye = NULL;

    uint8_t curr_entry = GET_FIRST_PARAM_ID(metadata);
    uint8_t next_entry;
    while (curr_entry != CAM_INTF_PARM_MAX) {
      switch (curr_entry) {
        case CAM_INTF_META_AEC_STATE:{
            uint8_t *ae_state =
                (uint8_t *)POINTER_OF(CAM_INTF_META_AEC_STATE, metadata);
            camMetadata.update(ANDROID_CONTROL_AE_STATE, ae_state, 1);
            ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AE_STATE", __func__);
            break;
        }
        case CAM_INTF_PARM_AEC_LOCK: {
            uint8_t  *ae_lock =
              (uint8_t *)POINTER_OF(CAM_INTF_PARM_AEC_LOCK, metadata);
            camMetadata.update(ANDROID_CONTROL_AE_LOCK,
                                          ae_lock, 1);
            ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AE_LOCK", __func__);
            break;
        }
        case CAM_INTF_PARM_FPS_RANGE: {
            int32_t fps_range[2];
            cam_fps_range_t * float_range =
              (cam_fps_range_t *)POINTER_OF(CAM_INTF_PARM_FPS_RANGE, metadata);
            fps_range[0] = (int32_t)float_range->min_fps;
            fps_range[1] = (int32_t)float_range->max_fps;
            camMetadata.update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                                          fps_range, 2);
            ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AE_TARGET_FPS_RANGE [%d, %d]",
                __func__, fps_range[0], fps_range[1]);
            break;
        }
        case CAM_INTF_PARM_EV: {
            int32_t  *expCompensation =
              (int32_t *)POINTER_OF(CAM_INTF_PARM_EV, metadata);
            camMetadata.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                          expCompensation, 1);
            ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION",
                __func__);
            break;
        }
        case CAM_INTF_PARM_FOCUS_MODE:{
            uint8_t  *focusMode =
                (uint8_t *)POINTER_OF(CAM_INTF_PARM_FOCUS_MODE, metadata);
            uint8_t fwkAfMode = (uint8_t)lookupFwkName(FOCUS_MODES_MAP,
               sizeof(FOCUS_MODES_MAP)/sizeof(FOCUS_MODES_MAP[0]), *focusMode);
            camMetadata.update(ANDROID_CONTROL_AF_MODE, &fwkAfMode, 1);
            ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AF_MODE", __func__);
            break;
        }
        case CAM_INTF_META_AF_STATE: {
            uint8_t  *afState =
               (uint8_t *)POINTER_OF(CAM_INTF_META_AF_STATE, metadata);
            camMetadata.update(ANDROID_CONTROL_AF_STATE, afState, 1);
            ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AF_STATE", __func__);
            break;
        }
        case CAM_INTF_PARM_WHITE_BALANCE: {
           uint8_t  *whiteBalance =
                (uint8_t *)POINTER_OF(CAM_INTF_PARM_WHITE_BALANCE, metadata);
             uint8_t fwkWhiteBalanceMode =
                    (uint8_t)lookupFwkName(WHITE_BALANCE_MODES_MAP,
                    sizeof(WHITE_BALANCE_MODES_MAP)/
                    sizeof(WHITE_BALANCE_MODES_MAP[0]), *whiteBalance);
             camMetadata.update(ANDROID_CONTROL_AWB_MODE,
                 &fwkWhiteBalanceMode, 1);
            ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AWB_MODE", __func__);
             break;
        }

        case CAM_INTF_META_AWB_STATE: {
           uint8_t  *whiteBalanceState =
              (uint8_t *)POINTER_OF(CAM_INTF_META_AWB_STATE, metadata);
           camMetadata.update(ANDROID_CONTROL_AWB_STATE, whiteBalanceState, 1);
           ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AWB_STATE", __func__);
           break;
        }


        case CAM_INTF_PARM_AWB_LOCK: {
            uint8_t  *awb_lock =
              (uint8_t *)POINTER_OF(CAM_INTF_PARM_AWB_LOCK, metadata);
            camMetadata.update(ANDROID_CONTROL_AWB_LOCK, awb_lock, 1);
            ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AWB_LOCK", __func__);
            break;
        }
        case CAM_INTF_META_PRECAPTURE_TRIGGER: {
            uint8_t *precaptureTrigger =
                (uint8_t *)POINTER_OF(CAM_INTF_META_PRECAPTURE_TRIGGER, metadata);
            camMetadata.update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER,
                 precaptureTrigger, 1);
            ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER",
                __func__);
            break;
        }
        case CAM_INTF_META_AF_TRIGGER_NOTICE: {
            uint8_t *af_trigger =
              (uint8_t *)POINTER_OF(CAM_INTF_META_AF_TRIGGER_NOTICE, metadata);
            camMetadata.update(ANDROID_CONTROL_AF_TRIGGER,
                af_trigger, 1);
            ALOGV("%s: urgent Metadata : ANDROID_CONTROL_AF_TRIGGER = %d",
                __func__, *af_trigger);
            break;
        }
        case CAM_INTF_META_AEC_MODE:{
            aeMode = (uint8_t*)
            POINTER_OF(CAM_INTF_META_AEC_MODE, metadata);
            break;
        }
        case CAM_INTF_PARM_LED_MODE:{
            flashMode = (int32_t*)
            POINTER_OF(CAM_INTF_PARM_LED_MODE, metadata);
            break;
        }
        case CAM_INTF_PARM_REDEYE_REDUCTION:{
            redeye = (int32_t*)
            POINTER_OF(CAM_INTF_PARM_REDEYE_REDUCTION, metadata);
            break;
        }
        default:
            ALOGV("%s: Normal Metadata %d, do not process",
              __func__, curr_entry);
            break;
       }
       next_entry = GET_NEXT_PARAM_ID(curr_entry, metadata);
       curr_entry = next_entry;
    }

    uint8_t fwk_aeMode;
    if (redeye != NULL && *redeye == 1) {
        fwk_aeMode = ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE;
        camMetadata.update(ANDROID_CONTROL_AE_MODE, &fwk_aeMode, 1);
    } else if (flashMode != NULL &&
            ((*flashMode == CAM_FLASH_MODE_AUTO)||
             (*flashMode == CAM_FLASH_MODE_ON))) {
        fwk_aeMode = (uint8_t)lookupFwkName(AE_FLASH_MODE_MAP,
                sizeof(AE_FLASH_MODE_MAP)/sizeof(AE_FLASH_MODE_MAP[0]),*flashMode);
        camMetadata.update(ANDROID_CONTROL_AE_MODE, &fwk_aeMode, 1);
    } else if (aeMode != NULL && *aeMode == CAM_AE_MODE_ON) {
        fwk_aeMode = ANDROID_CONTROL_AE_MODE_ON;
        camMetadata.update(ANDROID_CONTROL_AE_MODE, &fwk_aeMode, 1);
    } else if (aeMode != NULL && *aeMode == CAM_AE_MODE_OFF) {
        fwk_aeMode = ANDROID_CONTROL_AE_MODE_OFF;
        camMetadata.update(ANDROID_CONTROL_AE_MODE, &fwk_aeMode, 1);
    } else {
        ALOGE("%s: Not enough info to deduce ANDROID_CONTROL_AE_MODE redeye:%p, flashMode:%p, aeMode:%p!!!",__func__, redeye, flashMode, aeMode);
    }

    resultMetadata = camMetadata.release();
    return resultMetadata;
}

/*===========================================================================
 * FUNCTION   : dumpMetadataToFile
 *
 * DESCRIPTION: Dumps tuning metadata to file system
 *
 * PARAMETERS :
 *   @meta           : tuning metadata
 *   @dumpFrameCount : current dump frame count
 *   @enabled        : Enable mask
 *
 *==========================================================================*/
void QCamera3HardwareInterface::dumpMetadataToFile(tuning_params_t &meta,
                                                   uint32_t &dumpFrameCount,
                                                   int32_t enabled,
                                                   const char *type,
                                                   uint32_t frameNumber)
{
    uint32_t frm_num = 0;

    //Some sanity checks
    if (meta.tuning_sensor_data_size > TUNING_SENSOR_DATA_MAX) {
        ALOGE("%s : Tuning sensor data size bigger than expected %d: %d",
              __func__,
              meta.tuning_sensor_data_size,
              TUNING_SENSOR_DATA_MAX);
        return;
    }

    if (meta.tuning_vfe_data_size > TUNING_VFE_DATA_MAX) {
        ALOGE("%s : Tuning VFE data size bigger than expected %d: %d",
              __func__,
              meta.tuning_vfe_data_size,
              TUNING_VFE_DATA_MAX);
        return;
    }

    if (meta.tuning_cpp_data_size > TUNING_CPP_DATA_MAX) {
        ALOGE("%s : Tuning CPP data size bigger than expected %d: %d",
              __func__,
              meta.tuning_cpp_data_size,
              TUNING_CPP_DATA_MAX);
        return;
    }

    if (meta.tuning_cac_data_size > TUNING_CAC_DATA_MAX) {
        ALOGE("%s : Tuning CAC data size bigger than expected %d: %d",
              __func__,
              meta.tuning_cac_data_size,
              TUNING_CAC_DATA_MAX);
        return;
    }
    //

    if(enabled){
        frm_num = ((enabled & 0xffff0000) >> 16);
        if(frm_num == 0) {
            frm_num = 10; //default 10 frames
        }
        if(frm_num > 256) {
            frm_num = 256; //256 buffers cycle around
        }
        if((frm_num == 256) && (dumpFrameCount >= frm_num)) {
            // reset frame count if cycling
            dumpFrameCount = 0;
        }
        ALOGV("DumpFrmCnt = %d, frm_num = %d",dumpFrameCount, frm_num);
        if (dumpFrameCount < frm_num) {
            char timeBuf[FILENAME_MAX];
            char buf[FILENAME_MAX];
            memset(buf, 0, sizeof(buf));
            memset(timeBuf, 0, sizeof(timeBuf));
            time_t current_time;
            struct tm * timeinfo;
            time (&current_time);
            timeinfo = localtime (&current_time);
            strftime (timeBuf, sizeof(timeBuf),"/data/%Y%m%d%H%M%S", timeinfo);
            String8 filePath(timeBuf);
            snprintf(buf,
                     sizeof(buf),
                     "%d_HAL_META_%s_%d.bin",
                     dumpFrameCount,
                     type,
                     frameNumber);
            filePath.append(buf);
            int file_fd = open(filePath.string(), O_RDWR | O_CREAT, 0777);
            if (file_fd >= 0) {
                int written_len = 0;
                meta.tuning_data_version = TUNING_DATA_VERSION;
                void *data = (void *)((uint8_t *)&meta.tuning_data_version);
                written_len += write(file_fd, data, sizeof(uint32_t));
                data = (void *)((uint8_t *)&meta.tuning_sensor_data_size);
                ALOGV("tuning_sensor_data_size %d",(int)(*(int *)data));
                written_len += write(file_fd, data, sizeof(uint32_t));
                data = (void *)((uint8_t *)&meta.tuning_vfe_data_size);
                ALOGV("tuning_vfe_data_size %d",(int)(*(int *)data));
                written_len += write(file_fd, data, sizeof(uint32_t));
                data = (void *)((uint8_t *)&meta.tuning_cpp_data_size);
                ALOGV("tuning_cpp_data_size %d",(int)(*(int *)data));
                written_len += write(file_fd, data, sizeof(uint32_t));
                data = (void *)((uint8_t *)&meta.tuning_cac_data_size);
                ALOGV("tuning_cac_data_size %d",(int)(*(int *)data));
                written_len += write(file_fd, data, sizeof(uint32_t));
                int total_size = meta.tuning_sensor_data_size;
                data = (void *)((uint8_t *)&meta.data);
                written_len += write(file_fd, data, total_size);
                total_size = meta.tuning_vfe_data_size;
                data = (void *)((uint8_t *)&meta.data[TUNING_VFE_DATA_OFFSET]);
                written_len += write(file_fd, data, total_size);
                total_size = meta.tuning_cpp_data_size;
                data = (void *)((uint8_t *)&meta.data[TUNING_CPP_DATA_OFFSET]);
                written_len += write(file_fd, data, total_size);
                total_size = meta.tuning_cac_data_size;
                data = (void *)((uint8_t *)&meta.data[TUNING_CAC_DATA_OFFSET]);
                written_len += write(file_fd, data, total_size);
                close(file_fd);
            }else {
                ALOGE("%s: fail t open file for image dumping", __func__);
            }
            dumpFrameCount++;
        }
    }
}

/*===========================================================================
 * FUNCTION   : cleanAndSortStreamInfo
 *
 * DESCRIPTION: helper method to clean up invalid streams in stream_info,
 *              and sort them such that raw stream is at the end of the list
 *              This is a workaround for camera daemon constraint.
 *
 * PARAMETERS : None
 *
 *==========================================================================*/
void QCamera3HardwareInterface::cleanAndSortStreamInfo()
{
    List<stream_info_t *> newStreamInfo;

    /*clean up invalid streams*/
    for (List<stream_info_t*>::iterator it=mStreamInfo.begin();
            it != mStreamInfo.end();) {
        if(((*it)->status) == INVALID){
            QCamera3Channel *channel = (QCamera3Channel*)(*it)->stream->priv;
            delete channel;
            free(*it);
            it = mStreamInfo.erase(it);
        } else {
            it++;
        }
    }

    // Move preview/video/callback/snapshot streams into newList
    for (List<stream_info_t *>::iterator it = mStreamInfo.begin();
            it != mStreamInfo.end();) {
        if ((*it)->stream->format != HAL_PIXEL_FORMAT_RAW_OPAQUE &&
                (*it)->stream->format != HAL_PIXEL_FORMAT_RAW16) {
            newStreamInfo.push_back(*it);
            it = mStreamInfo.erase(it);
        } else
            it++;
    }
    // Move raw streams into newList
    for (List<stream_info_t *>::iterator it = mStreamInfo.begin();
            it != mStreamInfo.end();) {
        newStreamInfo.push_back(*it);
        it = mStreamInfo.erase(it);
    }

    mStreamInfo = newStreamInfo;
}

/*===========================================================================
 * FUNCTION   : extractJpegMetadata
 *
 * DESCRIPTION: helper method to extract Jpeg metadata from capture request.
 *              JPEG metadata is cached in HAL, and return as part of capture
 *              result when metadata is returned from camera daemon.
 *
 * PARAMETERS : @jpegMetadata: jpeg metadata to be extracted
 *              @request:      capture request
 *
 *==========================================================================*/
void QCamera3HardwareInterface::extractJpegMetadata(
        CameraMetadata& jpegMetadata,
        const camera3_capture_request_t *request)
{
    CameraMetadata frame_settings;
    frame_settings = request->settings;

    if (frame_settings.exists(ANDROID_JPEG_GPS_COORDINATES))
        jpegMetadata.update(ANDROID_JPEG_GPS_COORDINATES,
                frame_settings.find(ANDROID_JPEG_GPS_COORDINATES).data.d,
                frame_settings.find(ANDROID_JPEG_GPS_COORDINATES).count);

    if (frame_settings.exists(ANDROID_JPEG_GPS_PROCESSING_METHOD))
        jpegMetadata.update(ANDROID_JPEG_GPS_PROCESSING_METHOD,
                frame_settings.find(ANDROID_JPEG_GPS_PROCESSING_METHOD).data.u8,
                frame_settings.find(ANDROID_JPEG_GPS_PROCESSING_METHOD).count);

    if (frame_settings.exists(ANDROID_JPEG_GPS_TIMESTAMP))
        jpegMetadata.update(ANDROID_JPEG_GPS_TIMESTAMP,
                frame_settings.find(ANDROID_JPEG_GPS_TIMESTAMP).data.i64,
                frame_settings.find(ANDROID_JPEG_GPS_TIMESTAMP).count);

    if (frame_settings.exists(ANDROID_JPEG_ORIENTATION))
        jpegMetadata.update(ANDROID_JPEG_ORIENTATION,
                frame_settings.find(ANDROID_JPEG_ORIENTATION).data.i32,
                frame_settings.find(ANDROID_JPEG_ORIENTATION).count);

    if (frame_settings.exists(ANDROID_JPEG_QUALITY))
        jpegMetadata.update(ANDROID_JPEG_QUALITY,
                frame_settings.find(ANDROID_JPEG_QUALITY).data.u8,
                frame_settings.find(ANDROID_JPEG_QUALITY).count);

    if (frame_settings.exists(ANDROID_JPEG_THUMBNAIL_QUALITY))
        jpegMetadata.update(ANDROID_JPEG_THUMBNAIL_QUALITY,
                frame_settings.find(ANDROID_JPEG_THUMBNAIL_QUALITY).data.u8,
                frame_settings.find(ANDROID_JPEG_THUMBNAIL_QUALITY).count);

    if (frame_settings.exists(ANDROID_JPEG_THUMBNAIL_SIZE))
        jpegMetadata.update(ANDROID_JPEG_THUMBNAIL_SIZE,
                frame_settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32,
                frame_settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).count);
}

/*===========================================================================
 * FUNCTION   : convertToRegions
 *
 * DESCRIPTION: helper method to convert from cam_rect_t into int32_t array
 *
 * PARAMETERS :
 *   @rect   : cam_rect_t struct to convert
 *   @region : int32_t destination array
 *   @weight : if we are converting from cam_area_t, weight is valid
 *             else weight = -1
 *
 *==========================================================================*/
void QCamera3HardwareInterface::convertToRegions(cam_rect_t rect, int32_t* region, int weight){
    region[0] = rect.left;
    region[1] = rect.top;
    region[2] = rect.left + rect.width;
    region[3] = rect.top + rect.height;
    if (weight > -1) {
        region[4] = weight;
    }
}

/*===========================================================================
 * FUNCTION   : convertFromRegions
 *
 * DESCRIPTION: helper method to convert from array to cam_rect_t
 *
 * PARAMETERS :
 *   @rect   : cam_rect_t struct to convert
 *   @region : int32_t destination array
 *   @weight : if we are converting from cam_area_t, weight is valid
 *             else weight = -1
 *
 *==========================================================================*/
void QCamera3HardwareInterface::convertFromRegions(cam_area_t* roi,
                                                   const camera_metadata_t *settings,
                                                   uint32_t tag){
    CameraMetadata frame_settings;
    frame_settings = settings;
    int32_t x_min = frame_settings.find(tag).data.i32[0];
    int32_t y_min = frame_settings.find(tag).data.i32[1];
    int32_t x_max = frame_settings.find(tag).data.i32[2];
    int32_t y_max = frame_settings.find(tag).data.i32[3];
    roi->weight = frame_settings.find(tag).data.i32[4];
    roi->rect.left = x_min;
    roi->rect.top = y_min;
    roi->rect.width = x_max - x_min;
    roi->rect.height = y_max - y_min;
}

/*===========================================================================
 * FUNCTION   : resetIfNeededROI
 *
 * DESCRIPTION: helper method to reset the roi if it is greater than scaler
 *              crop region
 *
 * PARAMETERS :
 *   @roi       : cam_area_t struct to resize
 *   @scalerCropRegion : cam_crop_region_t region to compare against
 *
 *
 *==========================================================================*/
bool QCamera3HardwareInterface::resetIfNeededROI(cam_area_t* roi,
                                                 const cam_crop_region_t* scalerCropRegion)
{
    int32_t roi_x_max = roi->rect.width + roi->rect.left;
    int32_t roi_y_max = roi->rect.height + roi->rect.top;
    int32_t crop_x_max = scalerCropRegion->width + scalerCropRegion->left;
    int32_t crop_y_max = scalerCropRegion->height + scalerCropRegion->top;
    if ((roi_x_max < scalerCropRegion->left) ||
        (roi_y_max < scalerCropRegion->top)  ||
        (roi->rect.left > crop_x_max) ||
        (roi->rect.top > crop_y_max)){
        return false;
    }
    if (roi->rect.left < scalerCropRegion->left) {
        roi->rect.left = scalerCropRegion->left;
    }
    if (roi->rect.top < scalerCropRegion->top) {
        roi->rect.top = scalerCropRegion->top;
    }
    if (roi_x_max > crop_x_max) {
        roi_x_max = crop_x_max;
    }
    if (roi_y_max > crop_y_max) {
        roi_y_max = crop_y_max;
    }
    roi->rect.width = roi_x_max - roi->rect.left;
    roi->rect.height = roi_y_max - roi->rect.top;
    return true;
}

/*===========================================================================
 * FUNCTION   : convertLandmarks
 *
 * DESCRIPTION: helper method to extract the landmarks from face detection info
 *
 * PARAMETERS :
 *   @face   : cam_rect_t struct to convert
 *   @landmarks : int32_t destination array
 *
 *
 *==========================================================================*/
void QCamera3HardwareInterface::convertLandmarks(cam_face_detection_info_t face, int32_t* landmarks)
{
    landmarks[0] = face.left_eye_center.x;
    landmarks[1] = face.left_eye_center.y;
    landmarks[2] = face.right_eye_center.x;
    landmarks[3] = face.right_eye_center.y;
    landmarks[4] = face.mouth_center.x;
    landmarks[5] = face.mouth_center.y;
}

#define DATA_PTR(MEM_OBJ,INDEX) MEM_OBJ->getPtr( INDEX )
/*===========================================================================
 * FUNCTION   : initCapabilities
 *
 * DESCRIPTION: initialize camera capabilities in static data struct
 *
 * PARAMETERS :
 *   @cameraId  : camera Id
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::initCapabilities(int cameraId)
{
    int rc = 0;
    mm_camera_vtbl_t *cameraHandle = NULL;
    QCamera3HeapMemory *capabilityHeap = NULL;

    cameraHandle = camera_open(cameraId);
    if (!cameraHandle) {
        ALOGE("%s: camera_open failed", __func__);
        rc = -1;
        goto open_failed;
    }

    capabilityHeap = new QCamera3HeapMemory();
    if (capabilityHeap == NULL) {
        ALOGE("%s: creation of capabilityHeap failed", __func__);
        goto heap_creation_failed;
    }
    /* Allocate memory for capability buffer */
    rc = capabilityHeap->allocate(1, sizeof(cam_capability_t), false);
    if(rc != OK) {
        ALOGE("%s: No memory for cappability", __func__);
        goto allocate_failed;
    }

    /* Map memory for capability buffer */
    memset(DATA_PTR(capabilityHeap,0), 0, sizeof(cam_capability_t));
    rc = cameraHandle->ops->map_buf(cameraHandle->camera_handle,
                                CAM_MAPPING_BUF_TYPE_CAPABILITY,
                                capabilityHeap->getFd(0),
                                sizeof(cam_capability_t));
    if(rc < 0) {
        ALOGE("%s: failed to map capability buffer", __func__);
        goto map_failed;
    }

    /* Query Capability */
    rc = cameraHandle->ops->query_capability(cameraHandle->camera_handle);
    if(rc < 0) {
        ALOGE("%s: failed to query capability",__func__);
        goto query_failed;
    }
    gCamCapability[cameraId] = (cam_capability_t *)malloc(sizeof(cam_capability_t));
    if (!gCamCapability[cameraId]) {
        ALOGE("%s: out of memory", __func__);
        goto query_failed;
    }
    memcpy(gCamCapability[cameraId], DATA_PTR(capabilityHeap,0),
                                        sizeof(cam_capability_t));
    rc = 0;

query_failed:
    cameraHandle->ops->unmap_buf(cameraHandle->camera_handle,
                            CAM_MAPPING_BUF_TYPE_CAPABILITY);
map_failed:
    capabilityHeap->deallocate();
allocate_failed:
    delete capabilityHeap;
heap_creation_failed:
    cameraHandle->ops->close_camera(cameraHandle->camera_handle);
    cameraHandle = NULL;
open_failed:
    return rc;
}

/*===========================================================================
 * FUNCTION   : initParameters
 *
 * DESCRIPTION: initialize camera parameters
 *
 * PARAMETERS :
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::initParameters()
{
    int rc = 0;

    //Allocate Set Param Buffer
    mParamHeap = new QCamera3HeapMemory();
    rc = mParamHeap->allocate(1, sizeof(metadata_buffer_t), false);
    if(rc != OK) {
        rc = NO_MEMORY;
        ALOGE("Failed to allocate SETPARM Heap memory");
        delete mParamHeap;
        mParamHeap = NULL;
        return rc;
    }

    //Map memory for parameters buffer
    rc = mCameraHandle->ops->map_buf(mCameraHandle->camera_handle,
            CAM_MAPPING_BUF_TYPE_PARM_BUF,
            mParamHeap->getFd(0),
            sizeof(metadata_buffer_t));
    if(rc < 0) {
        ALOGE("%s:failed to map SETPARM buffer",__func__);
        rc = FAILED_TRANSACTION;
        mParamHeap->deallocate();
        delete mParamHeap;
        mParamHeap = NULL;
        return rc;
    }

    mParameters = (metadata_buffer_t*) DATA_PTR(mParamHeap,0);

    mPrevParameters = (metadata_buffer_t*)malloc(sizeof(metadata_buffer_t));
    return rc;
}

/*===========================================================================
 * FUNCTION   : deinitParameters
 *
 * DESCRIPTION: de-initialize camera parameters
 *
 * PARAMETERS :
 *
 * RETURN     : NONE
 *==========================================================================*/
void QCamera3HardwareInterface::deinitParameters()
{
    mCameraHandle->ops->unmap_buf(mCameraHandle->camera_handle,
            CAM_MAPPING_BUF_TYPE_PARM_BUF);

    mParamHeap->deallocate();
    delete mParamHeap;
    mParamHeap = NULL;

    mParameters = NULL;

    free(mPrevParameters);
    mPrevParameters = NULL;
}

/*===========================================================================
 * FUNCTION   : calcMaxJpegSize
 *
 * DESCRIPTION: Calculates maximum jpeg size supported by the cameraId
 *
 * PARAMETERS :
 *
 * RETURN     : max_jpeg_size
 *==========================================================================*/
int QCamera3HardwareInterface::calcMaxJpegSize()
{
    int32_t max_jpeg_size = 0;
    int temp_width, temp_height;
    for (int i = 0; i < gCamCapability[mCameraId]->picture_sizes_tbl_cnt; i++) {
        temp_width = gCamCapability[mCameraId]->picture_sizes_tbl[i].width;
        temp_height = gCamCapability[mCameraId]->picture_sizes_tbl[i].height;
        if (temp_width * temp_height > max_jpeg_size ) {
            max_jpeg_size = temp_width * temp_height;
        }
    }
    max_jpeg_size = max_jpeg_size * 3/2 + sizeof(camera3_jpeg_blob_t);
    return max_jpeg_size;
}

/*===========================================================================
 * FUNCTION   : initStaticMetadata
 *
 * DESCRIPTION: initialize the static metadata
 *
 * PARAMETERS :
 *   @cameraId  : camera Id
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              non-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::initStaticMetadata(int cameraId)
{
    int rc = 0;
    CameraMetadata staticInfo;

    int facingBack = gCamCapability[cameraId]->position == CAM_POSITION_BACK;

     /* android.info: hardware level */
    uint8_t supportedHardwareLevel = (facingBack)? ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
      ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    staticInfo.update(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
        &supportedHardwareLevel, 1);
    /*HAL 3 only*/
    staticInfo.update(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
                    &gCamCapability[cameraId]->min_focus_distance, 1);

    staticInfo.update(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
                    &gCamCapability[cameraId]->hyper_focal_distance, 1);

    /*should be using focal lengths but sensor doesn't provide that info now*/
    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
                      &gCamCapability[cameraId]->focal_length,
                      1);

    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_APERTURES,
                      gCamCapability[cameraId]->apertures,
                      gCamCapability[cameraId]->apertures_count);

    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
                gCamCapability[cameraId]->filter_densities,
                gCamCapability[cameraId]->filter_densities_count);


    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
                      (uint8_t*)gCamCapability[cameraId]->optical_stab_modes,
                      gCamCapability[cameraId]->optical_stab_modes_count);

    staticInfo.update(ANDROID_LENS_POSITION,
                      gCamCapability[cameraId]->lens_position,
                      sizeof(gCamCapability[cameraId]->lens_position)/ sizeof(float));

    int32_t lens_shading_map_size[] = {gCamCapability[cameraId]->lens_shading_map_size.width,
                                       gCamCapability[cameraId]->lens_shading_map_size.height};
    staticInfo.update(ANDROID_LENS_INFO_SHADING_MAP_SIZE,
                      lens_shading_map_size,
                      sizeof(lens_shading_map_size)/sizeof(int32_t));

    staticInfo.update(ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
            gCamCapability[cameraId]->sensor_physical_size, 2);

    staticInfo.update(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
            gCamCapability[cameraId]->exposure_time_range, 2);

    staticInfo.update(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
            &gCamCapability[cameraId]->max_frame_duration, 1);

    camera_metadata_rational baseGainFactor = {
            gCamCapability[cameraId]->base_gain_factor.numerator,
            gCamCapability[cameraId]->base_gain_factor.denominator};
    staticInfo.update(ANDROID_SENSOR_BASE_GAIN_FACTOR,
                      &baseGainFactor, 1);

    staticInfo.update(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
                     (uint8_t*)&gCamCapability[cameraId]->color_arrangement, 1);

    int32_t pixel_array_size[] = {gCamCapability[cameraId]->pixel_array_size.width,
                                  gCamCapability[cameraId]->pixel_array_size.height};
    staticInfo.update(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE,
                      pixel_array_size, 2);

    int32_t active_array_size[] = {gCamCapability[cameraId]->active_array_size.left,
                                                gCamCapability[cameraId]->active_array_size.top,
                                                gCamCapability[cameraId]->active_array_size.width,
                                                gCamCapability[cameraId]->active_array_size.height};
    staticInfo.update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                      active_array_size, 4);

    staticInfo.update(ANDROID_SENSOR_INFO_WHITE_LEVEL,
            &gCamCapability[cameraId]->white_level, 1);

    staticInfo.update(ANDROID_SENSOR_BLACK_LEVEL_PATTERN,
            gCamCapability[cameraId]->black_level_pattern, 4);

    staticInfo.update(ANDROID_FLASH_INFO_CHARGE_DURATION,
                      &gCamCapability[cameraId]->flash_charge_duration, 1);

    staticInfo.update(ANDROID_TONEMAP_MAX_CURVE_POINTS,
                      &gCamCapability[cameraId]->max_tone_map_curve_points, 1);

    int32_t maxFaces = gCamCapability[cameraId]->max_num_roi;
    staticInfo.update(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
                      (int32_t*)&maxFaces, 1);

    staticInfo.update(ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
                      &gCamCapability[cameraId]->histogram_size, 1);

    staticInfo.update(ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
            &gCamCapability[cameraId]->max_histogram_count, 1);

    int32_t sharpness_map_size[] = {gCamCapability[cameraId]->sharpness_map_size.width,
                                    gCamCapability[cameraId]->sharpness_map_size.height};

    staticInfo.update(ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
            sharpness_map_size, sizeof(sharpness_map_size)/sizeof(int32_t));

    staticInfo.update(ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
            &gCamCapability[cameraId]->max_sharpness_map_value, 1);

    int32_t scalar_formats[] = {
            ANDROID_SCALER_AVAILABLE_FORMATS_RAW_OPAQUE,
            ANDROID_SCALER_AVAILABLE_FORMATS_RAW16,
            ANDROID_SCALER_AVAILABLE_FORMATS_YCbCr_420_888,
            ANDROID_SCALER_AVAILABLE_FORMATS_BLOB,
            HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED};
    int scalar_formats_count = sizeof(scalar_formats)/sizeof(int32_t);
    staticInfo.update(ANDROID_SCALER_AVAILABLE_FORMATS,
                      scalar_formats,
                      scalar_formats_count);

    int32_t available_processed_sizes[MAX_SIZES_CNT * 2];
    makeTable(gCamCapability[cameraId]->picture_sizes_tbl,
              gCamCapability[cameraId]->picture_sizes_tbl_cnt,
              available_processed_sizes);
    staticInfo.update(ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES,
                available_processed_sizes,
                (gCamCapability[cameraId]->picture_sizes_tbl_cnt) * 2);

    int32_t available_raw_sizes[MAX_SIZES_CNT * 2];
    makeTable(gCamCapability[cameraId]->raw_dim,
              gCamCapability[cameraId]->supported_raw_dim_cnt,
              available_raw_sizes);
    staticInfo.update(ANDROID_SCALER_AVAILABLE_RAW_SIZES,
                available_raw_sizes,
                gCamCapability[cameraId]->supported_raw_dim_cnt * 2);

    int32_t available_fps_ranges[MAX_SIZES_CNT * 2];
    makeFPSTable(gCamCapability[cameraId]->fps_ranges_tbl,
                 gCamCapability[cameraId]->fps_ranges_tbl_cnt,
                 available_fps_ranges);
    staticInfo.update(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
            available_fps_ranges, (gCamCapability[cameraId]->fps_ranges_tbl_cnt*2) );

    camera_metadata_rational exposureCompensationStep = {
            gCamCapability[cameraId]->exp_compensation_step.numerator,
            gCamCapability[cameraId]->exp_compensation_step.denominator};
    staticInfo.update(ANDROID_CONTROL_AE_COMPENSATION_STEP,
                      &exposureCompensationStep, 1);

    /*TO DO*/
    uint8_t availableVstabModes[] = {ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF};
    staticInfo.update(ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
                      availableVstabModes, sizeof(availableVstabModes));

    /*HAL 1 and HAL 3 common*/
    float maxZoom = 4;
    staticInfo.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
            &maxZoom, 1);

    uint8_t croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    staticInfo.update(ANDROID_SCALER_CROPPING_TYPE, &croppingType, 1);

    int32_t max3aRegions[3] = {/*AE*/1,/*AWB*/ 0,/*AF*/ 1};
    if (gCamCapability[cameraId]->supported_focus_modes_cnt == 1)
        max3aRegions[2] = 0; /* AF not supported */
    staticInfo.update(ANDROID_CONTROL_MAX_REGIONS,
            max3aRegions, 3);

    uint8_t availableFaceDetectModes[] = {
            ANDROID_STATISTICS_FACE_DETECT_MODE_OFF,
            ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE };
    staticInfo.update(ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
                      availableFaceDetectModes,
                      sizeof(availableFaceDetectModes));

    int32_t exposureCompensationRange[] = {gCamCapability[cameraId]->exposure_compensation_min,
                                           gCamCapability[cameraId]->exposure_compensation_max};
    staticInfo.update(ANDROID_CONTROL_AE_COMPENSATION_RANGE,
            exposureCompensationRange,
            sizeof(exposureCompensationRange)/sizeof(int32_t));

    uint8_t lensFacing = (facingBack) ?
            ANDROID_LENS_FACING_BACK : ANDROID_LENS_FACING_FRONT;
    staticInfo.update(ANDROID_LENS_FACING, &lensFacing, 1);

    staticInfo.update(ANDROID_SCALER_AVAILABLE_JPEG_SIZES,
                available_processed_sizes,
                (gCamCapability[cameraId]->picture_sizes_tbl_cnt * 2));

    staticInfo.update(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
                      available_thumbnail_sizes,
                      sizeof(available_thumbnail_sizes)/sizeof(int32_t));

    /*android.scaler.availableStreamConfigurations*/
    int32_t max_stream_configs_size =
            gCamCapability[cameraId]->picture_sizes_tbl_cnt *
            sizeof(scalar_formats)/sizeof(int32_t) * 4;
    int32_t available_stream_configs[max_stream_configs_size];
    int idx = 0;
    for (int j = 0; j < scalar_formats_count; j++) {
        switch (scalar_formats[j]) {
        case ANDROID_SCALER_AVAILABLE_FORMATS_RAW16:
        case ANDROID_SCALER_AVAILABLE_FORMATS_RAW_OPAQUE:
            for (int i = 0;
                i < gCamCapability[cameraId]->supported_raw_dim_cnt; i++) {
                available_stream_configs[idx] = scalar_formats[j];
                available_stream_configs[idx+1] =
                    gCamCapability[cameraId]->raw_dim[i].width;
                available_stream_configs[idx+2] =
                    gCamCapability[cameraId]->raw_dim[i].height;
                available_stream_configs[idx+3] =
                    ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT;
                idx+=4;
            }
            break;
        default:
            for (int i = 0;
                i < gCamCapability[cameraId]->picture_sizes_tbl_cnt; i++) {
                available_stream_configs[idx] = scalar_formats[j];
                available_stream_configs[idx+1] =
                    gCamCapability[cameraId]->picture_sizes_tbl[i].width;
                available_stream_configs[idx+2] =
                    gCamCapability[cameraId]->picture_sizes_tbl[i].height;
                available_stream_configs[idx+3] =
                    ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT;
                idx+=4;
            }


            break;
        }
    }
    staticInfo.update(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                      available_stream_configs, idx);
    static const uint8_t hotpixelMode = ANDROID_HOT_PIXEL_MODE_FAST;
    staticInfo.update(ANDROID_HOT_PIXEL_MODE, &hotpixelMode, 1);

    static const uint8_t hotPixelMapMode = ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_OFF;
    staticInfo.update(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE, &hotPixelMapMode, 1);

    /* android.scaler.availableMinFrameDurations */
    int64_t available_min_durations[max_stream_configs_size];
    idx = 0;
    for (int j = 0; j < scalar_formats_count; j++) {
        switch (scalar_formats[j]) {
        case ANDROID_SCALER_AVAILABLE_FORMATS_RAW16:
        case ANDROID_SCALER_AVAILABLE_FORMATS_RAW_OPAQUE:
            for (int i = 0;
                i < gCamCapability[cameraId]->supported_raw_dim_cnt; i++) {
                available_min_durations[idx] = scalar_formats[j];
                available_min_durations[idx+1] =
                    gCamCapability[cameraId]->raw_dim[i].width;
                available_min_durations[idx+2] =
                    gCamCapability[cameraId]->raw_dim[i].height;
                available_min_durations[idx+3] =
                    gCamCapability[cameraId]->raw_min_duration[i];
                idx+=4;
            }
            break;
        default:
            for (int i = 0;
                i < gCamCapability[cameraId]->picture_sizes_tbl_cnt; i++) {
                available_min_durations[idx] = scalar_formats[j];
                available_min_durations[idx+1] =
                    gCamCapability[cameraId]->picture_sizes_tbl[i].width;
                available_min_durations[idx+2] =
                    gCamCapability[cameraId]->picture_sizes_tbl[i].height;
                available_min_durations[idx+3] =
                    gCamCapability[cameraId]->picture_min_duration[i];
                idx+=4;
            }
            break;
        }
    }
    staticInfo.update(ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
                      &available_min_durations[0], idx);

    int32_t max_jpeg_size = 0;
    int temp_width, temp_height;
    for (int i = 0; i < gCamCapability[cameraId]->picture_sizes_tbl_cnt; i++) {
        temp_width = gCamCapability[cameraId]->picture_sizes_tbl[i].width;
        temp_height = gCamCapability[cameraId]->picture_sizes_tbl[i].height;
        if (temp_width * temp_height > max_jpeg_size ) {
            max_jpeg_size = temp_width * temp_height;
        }
    }
    max_jpeg_size = max_jpeg_size * 3/2 + sizeof(camera3_jpeg_blob_t);
    staticInfo.update(ANDROID_JPEG_MAX_SIZE,
                      &max_jpeg_size, 1);

    uint8_t avail_effects[CAM_EFFECT_MODE_MAX];
    size_t size = 0;
    for (int i = 0; i < gCamCapability[cameraId]->supported_effects_cnt; i++) {
        int32_t val = lookupFwkName(EFFECT_MODES_MAP,
                                   sizeof(EFFECT_MODES_MAP)/sizeof(EFFECT_MODES_MAP[0]),
                                   gCamCapability[cameraId]->supported_effects[i]);
        if (val != NAME_NOT_FOUND) {
            avail_effects[size] = (uint8_t)val;
            size++;
        }
    }
    staticInfo.update(ANDROID_CONTROL_AVAILABLE_EFFECTS,
                      avail_effects,
                      size);

    uint8_t avail_scene_modes[CAM_SCENE_MODE_MAX];
    uint8_t supported_indexes[CAM_SCENE_MODE_MAX];
    int32_t supported_scene_modes_cnt = 0;
    for (int i = 0; i < gCamCapability[cameraId]->supported_scene_modes_cnt; i++) {
        int32_t val = lookupFwkName(SCENE_MODES_MAP,
                                sizeof(SCENE_MODES_MAP)/sizeof(SCENE_MODES_MAP[0]),
                                gCamCapability[cameraId]->supported_scene_modes[i]);
        if (val != NAME_NOT_FOUND) {
            avail_scene_modes[supported_scene_modes_cnt] = (uint8_t)val;
            supported_indexes[supported_scene_modes_cnt] = i;
            supported_scene_modes_cnt++;
        }
    }

    staticInfo.update(ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
                      avail_scene_modes,
                      supported_scene_modes_cnt);

    uint8_t scene_mode_overrides[CAM_SCENE_MODE_MAX * 3];
    makeOverridesList(gCamCapability[cameraId]->scene_mode_overrides,
                      supported_scene_modes_cnt,
                      scene_mode_overrides,
                      supported_indexes,
                      cameraId);
    staticInfo.update(ANDROID_CONTROL_SCENE_MODE_OVERRIDES,
                      scene_mode_overrides,
                      supported_scene_modes_cnt*3);

    uint8_t avail_antibanding_modes[CAM_ANTIBANDING_MODE_MAX];
    size = 0;
    for (int i = 0; i < gCamCapability[cameraId]->supported_antibandings_cnt; i++) {
        int32_t val = lookupFwkName(ANTIBANDING_MODES_MAP,
                                 sizeof(ANTIBANDING_MODES_MAP)/sizeof(ANTIBANDING_MODES_MAP[0]),
                                 gCamCapability[cameraId]->supported_antibandings[i]);
        if (val != NAME_NOT_FOUND) {
            avail_antibanding_modes[size] = (uint8_t)val;
            size++;
        }

    }
    staticInfo.update(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
                      avail_antibanding_modes,
                      size);

    uint8_t avail_af_modes[CAM_FOCUS_MODE_MAX];
    size = 0;
    for (int i = 0; i < gCamCapability[cameraId]->supported_focus_modes_cnt; i++) {
        int32_t val = lookupFwkName(FOCUS_MODES_MAP,
                                sizeof(FOCUS_MODES_MAP)/sizeof(FOCUS_MODES_MAP[0]),
                                gCamCapability[cameraId]->supported_focus_modes[i]);
        if (val != NAME_NOT_FOUND) {
            avail_af_modes[size] = (uint8_t)val;
            size++;
        }
    }
    staticInfo.update(ANDROID_CONTROL_AF_AVAILABLE_MODES,
                      avail_af_modes,
                      size);

    uint8_t avail_awb_modes[CAM_WB_MODE_MAX];
    size = 0;
    for (int i = 0; i < gCamCapability[cameraId]->supported_white_balances_cnt; i++) {
        int32_t val = lookupFwkName(WHITE_BALANCE_MODES_MAP,
                                    sizeof(WHITE_BALANCE_MODES_MAP)/sizeof(WHITE_BALANCE_MODES_MAP[0]),
                                    gCamCapability[cameraId]->supported_white_balances[i]);
        if (val != NAME_NOT_FOUND) {
            avail_awb_modes[size] = (uint8_t)val;
            size++;
        }
    }
    staticInfo.update(ANDROID_CONTROL_AWB_AVAILABLE_MODES,
                      avail_awb_modes,
                      size);

    uint8_t available_flash_levels[CAM_FLASH_FIRING_LEVEL_MAX];
    for (int i = 0; i < gCamCapability[cameraId]->supported_flash_firing_level_cnt; i++)
      available_flash_levels[i] = gCamCapability[cameraId]->supported_firing_levels[i];

    staticInfo.update(ANDROID_FLASH_FIRING_POWER,
            available_flash_levels,
            gCamCapability[cameraId]->supported_flash_firing_level_cnt);

    uint8_t flashAvailable;
    if (gCamCapability[cameraId]->flash_available)
        flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    else
        flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    staticInfo.update(ANDROID_FLASH_INFO_AVAILABLE,
            &flashAvailable, 1);

    uint8_t avail_ae_modes[5];
    size = 0;
    for (int i = 0; i < gCamCapability[cameraId]->supported_ae_modes_cnt; i++) {
        avail_ae_modes[i] = gCamCapability[cameraId]->supported_ae_modes[i];
        size++;
    }
    if (flashAvailable) {
        avail_ae_modes[size++] = ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH;
        avail_ae_modes[size++] = ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH;
        avail_ae_modes[size++] = ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE;
    }
    staticInfo.update(ANDROID_CONTROL_AE_AVAILABLE_MODES,
                      avail_ae_modes,
                      size);

    int32_t sensitivity_range[2];
    sensitivity_range[0] = gCamCapability[cameraId]->sensitivity_range.min_sensitivity;
    sensitivity_range[1] = gCamCapability[cameraId]->sensitivity_range.max_sensitivity;
    staticInfo.update(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE,
                      sensitivity_range,
                      sizeof(sensitivity_range) / sizeof(int32_t));

    staticInfo.update(ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY,
                      &gCamCapability[cameraId]->max_analog_sensitivity,
                      1);

    int32_t sensor_orientation = (int32_t)gCamCapability[cameraId]->sensor_mount_angle;
    staticInfo.update(ANDROID_SENSOR_ORIENTATION,
                      &sensor_orientation,
                      1);

    int32_t max_output_streams[3] = {1, 3, 1};
    staticInfo.update(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS,
                      max_output_streams,
                      3);

    uint8_t avail_leds = 0;
    staticInfo.update(ANDROID_LED_AVAILABLE_LEDS,
                      &avail_leds, 0);

    uint8_t focus_dist_calibrated;
    int32_t val = lookupFwkName(FOCUS_CALIBRATION_MAP,
            sizeof(FOCUS_CALIBRATION_MAP)/sizeof(FOCUS_CALIBRATION_MAP[0]),
            gCamCapability[cameraId]->focus_dist_calibrated);
    if (val != NAME_NOT_FOUND) {
        focus_dist_calibrated = (uint8_t)val;
        staticInfo.update(ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,
                     &focus_dist_calibrated, 1);
    }

    int32_t avail_testpattern_modes[MAX_TEST_PATTERN_CNT];
    size = 0;
    for (int i = 0; i < gCamCapability[cameraId]->supported_test_pattern_modes_cnt;
            i++) {
        int32_t val = lookupFwkName(TEST_PATTERN_MAP,
                                    sizeof(TEST_PATTERN_MAP)/sizeof(TEST_PATTERN_MAP[0]),
                                    gCamCapability[cameraId]->supported_test_pattern_modes[i]);
        if (val != NAME_NOT_FOUND) {
            avail_testpattern_modes[size] = val;
            size++;
        }
    }
    staticInfo.update(ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES,
                      avail_testpattern_modes,
                      size);

    uint8_t max_pipeline_depth = kMaxInFlight + EMPTY_PIPELINE_DELAY;
    staticInfo.update(ANDROID_REQUEST_PIPELINE_MAX_DEPTH,
                      &max_pipeline_depth,
                      1);

    int32_t partial_result_count = 2;
    staticInfo.update(ANDROID_REQUEST_PARTIAL_RESULT_COUNT,
                      &partial_result_count,
                       1);

    uint8_t available_capabilities[MAX_AVAILABLE_CAPABILITIES];
    uint8_t available_capabilities_count = 0;
    available_capabilities[available_capabilities_count++] = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE;
    available_capabilities[available_capabilities_count++] = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR;
    available_capabilities[available_capabilities_count++] = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_POST_PROCESSING;
    available_capabilities[available_capabilities_count++] = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_READ_SENSOR_SETTINGS;
    available_capabilities[available_capabilities_count++] = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE;
    if (facingBack) {
        available_capabilities[available_capabilities_count++] = ANDROID_REQUEST_AVAILABLE_CAPABILITIES_RAW;
    }
    staticInfo.update(ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
                      available_capabilities,
                      available_capabilities_count);

    int32_t max_input_streams = 0;
    staticInfo.update(ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS,
                      &max_input_streams,
                      1);

    int32_t io_format_map[] = {};
    staticInfo.update(ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP,
                      io_format_map, 0);

    int32_t max_latency = (facingBack)? ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL:CAM_MAX_SYNC_LATENCY;
    staticInfo.update(ANDROID_SYNC_MAX_LATENCY,
                      &max_latency,
                      1);

    float optical_axis_angle[2];
    optical_axis_angle[0] = 0; //need to verify
    optical_axis_angle[1] = 0; //need to verify
    staticInfo.update(ANDROID_LENS_OPTICAL_AXIS_ANGLE,
                      optical_axis_angle,
                      2);

    uint8_t available_hot_pixel_modes[] = {ANDROID_HOT_PIXEL_MODE_FAST};
    staticInfo.update(ANDROID_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES,
                      available_hot_pixel_modes,
                      1);

    uint8_t available_edge_modes[] = {ANDROID_EDGE_MODE_OFF,
                                      ANDROID_EDGE_MODE_FAST};
    staticInfo.update(ANDROID_EDGE_AVAILABLE_EDGE_MODES,
                      available_edge_modes,
                      2);

    uint8_t available_noise_red_modes[] = {ANDROID_NOISE_REDUCTION_MODE_OFF,
                                           ANDROID_NOISE_REDUCTION_MODE_FAST};
    staticInfo.update(ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES,
                      available_noise_red_modes,
                      2);

    uint8_t available_tonemap_modes[] = {ANDROID_TONEMAP_MODE_CONTRAST_CURVE,
                                         ANDROID_TONEMAP_MODE_FAST};
    staticInfo.update(ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES,
                      available_tonemap_modes,
                      2);

    uint8_t available_hot_pixel_map_modes[] = {ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_OFF};
    staticInfo.update(ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES,
                      available_hot_pixel_map_modes,
                      1);

    uint8_t fwkReferenceIlluminant = lookupFwkName(REFERENCE_ILLUMINANT_MAP,
        sizeof(REFERENCE_ILLUMINANT_MAP) / sizeof(REFERENCE_ILLUMINANT_MAP[0]),
        gCamCapability[cameraId]->reference_illuminant1);
    staticInfo.update(ANDROID_SENSOR_REFERENCE_ILLUMINANT1,
                      &fwkReferenceIlluminant, 1);

    fwkReferenceIlluminant = lookupFwkName(REFERENCE_ILLUMINANT_MAP,
        sizeof(REFERENCE_ILLUMINANT_MAP) / sizeof(REFERENCE_ILLUMINANT_MAP[0]),
        gCamCapability[cameraId]->reference_illuminant2);
    staticInfo.update(ANDROID_SENSOR_REFERENCE_ILLUMINANT2,
                      &fwkReferenceIlluminant, 1);

    staticInfo.update(ANDROID_SENSOR_FORWARD_MATRIX1,
                      (camera_metadata_rational_t*)gCamCapability[cameraId]->forward_matrix1,
                      3*3);

    staticInfo.update(ANDROID_SENSOR_FORWARD_MATRIX2,
                      (camera_metadata_rational_t*)gCamCapability[cameraId]->forward_matrix2,
                      3*3);

    staticInfo.update(ANDROID_SENSOR_COLOR_TRANSFORM1,
                   (camera_metadata_rational_t*) gCamCapability[cameraId]->color_transform1,
                      3*3);

    staticInfo.update(ANDROID_SENSOR_COLOR_TRANSFORM2,
                   (camera_metadata_rational_t*) gCamCapability[cameraId]->color_transform2,
                      3*3);

    staticInfo.update(ANDROID_SENSOR_CALIBRATION_TRANSFORM1,
                   (camera_metadata_rational_t*) gCamCapability[cameraId]->calibration_transform1,
                      3*3);

    staticInfo.update(ANDROID_SENSOR_CALIBRATION_TRANSFORM2,
                   (camera_metadata_rational_t*) gCamCapability[cameraId]->calibration_transform2,
                      3*3);

    int32_t request_keys_basic[] = {ANDROID_COLOR_CORRECTION_MODE,
       ANDROID_COLOR_CORRECTION_TRANSFORM, ANDROID_COLOR_CORRECTION_GAINS,
       ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
       ANDROID_CONTROL_AE_ANTIBANDING_MODE, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
       ANDROID_CONTROL_AE_LOCK, ANDROID_CONTROL_AE_MODE,
       ANDROID_CONTROL_AE_REGIONS, ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
       ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, ANDROID_CONTROL_AF_MODE,
       ANDROID_CONTROL_AF_TRIGGER, ANDROID_CONTROL_AWB_LOCK,
       ANDROID_CONTROL_AWB_MODE, ANDROID_CONTROL_CAPTURE_INTENT,
       ANDROID_CONTROL_EFFECT_MODE, ANDROID_CONTROL_MODE,
       ANDROID_CONTROL_SCENE_MODE, ANDROID_CONTROL_VIDEO_STABILIZATION_MODE,
       ANDROID_DEMOSAIC_MODE, ANDROID_EDGE_MODE, ANDROID_EDGE_STRENGTH,
       ANDROID_FLASH_FIRING_POWER, ANDROID_FLASH_FIRING_TIME, ANDROID_FLASH_MODE,
       ANDROID_JPEG_GPS_COORDINATES,
       ANDROID_JPEG_GPS_PROCESSING_METHOD, ANDROID_JPEG_GPS_TIMESTAMP,
       ANDROID_JPEG_ORIENTATION, ANDROID_JPEG_QUALITY, ANDROID_JPEG_THUMBNAIL_QUALITY,
       ANDROID_JPEG_THUMBNAIL_SIZE, ANDROID_LENS_APERTURE, ANDROID_LENS_FILTER_DENSITY,
       ANDROID_LENS_FOCAL_LENGTH, ANDROID_LENS_FOCUS_DISTANCE,
       ANDROID_LENS_OPTICAL_STABILIZATION_MODE, ANDROID_NOISE_REDUCTION_MODE,
       ANDROID_NOISE_REDUCTION_STRENGTH, ANDROID_REQUEST_ID, ANDROID_REQUEST_TYPE,
       ANDROID_SCALER_CROP_REGION, ANDROID_SENSOR_EXPOSURE_TIME,
       ANDROID_SENSOR_FRAME_DURATION, ANDROID_HOT_PIXEL_MODE,
       ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE,
       ANDROID_SENSOR_SENSITIVITY, ANDROID_SHADING_MODE,
       ANDROID_SHADING_STRENGTH, ANDROID_STATISTICS_FACE_DETECT_MODE,
       ANDROID_STATISTICS_HISTOGRAM_MODE, ANDROID_STATISTICS_SHARPNESS_MAP_MODE,
       ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, ANDROID_TONEMAP_CURVE_BLUE,
       ANDROID_TONEMAP_CURVE_GREEN, ANDROID_TONEMAP_CURVE_RED, ANDROID_TONEMAP_MODE,
       ANDROID_BLACK_LEVEL_LOCK };

    size_t request_keys_cnt =
            sizeof(request_keys_basic)/sizeof(request_keys_basic[0]);
    //NOTE: Please increase available_request_keys array size before
    //adding any new entries.
    int32_t available_request_keys[request_keys_cnt+1];
    memcpy(available_request_keys, request_keys_basic,
            sizeof(request_keys_basic));
    if (gCamCapability[cameraId]->supported_focus_modes_cnt > 1) {
        available_request_keys[request_keys_cnt++] =
                ANDROID_CONTROL_AF_REGIONS;
    }
    //NOTE: Please increase available_request_keys array size before
    //adding any new entries.
    staticInfo.update(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS,
                      available_request_keys, request_keys_cnt);

    int32_t result_keys_basic[] = {ANDROID_COLOR_CORRECTION_TRANSFORM,
       ANDROID_COLOR_CORRECTION_GAINS, ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
       ANDROID_CONTROL_AE_MODE, ANDROID_CONTROL_AE_REGIONS,
       ANDROID_CONTROL_AE_STATE, ANDROID_CONTROL_AF_MODE,
       ANDROID_CONTROL_AF_STATE, ANDROID_CONTROL_AWB_MODE,
       ANDROID_CONTROL_AWB_STATE, ANDROID_CONTROL_MODE, ANDROID_EDGE_MODE,
       ANDROID_FLASH_FIRING_POWER, ANDROID_FLASH_FIRING_TIME, ANDROID_FLASH_MODE,
       ANDROID_FLASH_STATE, ANDROID_JPEG_GPS_COORDINATES, ANDROID_JPEG_GPS_PROCESSING_METHOD,
       ANDROID_JPEG_GPS_TIMESTAMP, ANDROID_JPEG_ORIENTATION, ANDROID_JPEG_QUALITY,
       ANDROID_JPEG_THUMBNAIL_QUALITY, ANDROID_JPEG_THUMBNAIL_SIZE, ANDROID_LENS_APERTURE,
       ANDROID_LENS_FILTER_DENSITY, ANDROID_LENS_FOCAL_LENGTH, ANDROID_LENS_FOCUS_DISTANCE,
       ANDROID_LENS_FOCUS_RANGE, ANDROID_LENS_STATE, ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
       ANDROID_NOISE_REDUCTION_MODE, ANDROID_REQUEST_ID,
       ANDROID_SCALER_CROP_REGION, ANDROID_SHADING_MODE, ANDROID_SENSOR_EXPOSURE_TIME,
       ANDROID_SENSOR_FRAME_DURATION, ANDROID_SENSOR_SENSITIVITY,
       ANDROID_SENSOR_TIMESTAMP, ANDROID_SENSOR_NEUTRAL_COLOR_POINT,
       ANDROID_SENSOR_PROFILE_TONE_CURVE, ANDROID_BLACK_LEVEL_LOCK, ANDROID_TONEMAP_CURVE_BLUE,
       ANDROID_TONEMAP_CURVE_GREEN, ANDROID_TONEMAP_CURVE_RED, ANDROID_TONEMAP_MODE,
       ANDROID_STATISTICS_FACE_DETECT_MODE, ANDROID_STATISTICS_HISTOGRAM_MODE,
       ANDROID_STATISTICS_SHARPNESS_MAP, ANDROID_STATISTICS_SHARPNESS_MAP_MODE,
       ANDROID_STATISTICS_PREDICTED_COLOR_GAINS, ANDROID_STATISTICS_PREDICTED_COLOR_TRANSFORM,
       ANDROID_STATISTICS_SCENE_FLICKER, ANDROID_STATISTICS_FACE_IDS,
       ANDROID_STATISTICS_FACE_LANDMARKS, ANDROID_STATISTICS_FACE_RECTANGLES,
       ANDROID_STATISTICS_FACE_SCORES,
       ANDROID_SENSOR_NOISE_PROFILE,
       ANDROID_SENSOR_GREEN_SPLIT};
    size_t result_keys_cnt =
            sizeof(result_keys_basic)/sizeof(result_keys_basic[0]);
    //NOTE: Please increase available_result_keys array size before
    //adding any new entries.
    int32_t available_result_keys[result_keys_cnt+1];
    memcpy(available_result_keys, result_keys_basic,
            sizeof(result_keys_basic));
    if (gCamCapability[cameraId]->supported_focus_modes_cnt > 1) {
        available_result_keys[result_keys_cnt++] =
                ANDROID_CONTROL_AF_REGIONS;
    }
    //NOTE: Please increase available_result_keys array size before
    //adding any new entries.

    staticInfo.update(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS,
                      available_result_keys, result_keys_cnt);

    int32_t available_characteristics_keys[] = {ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
       ANDROID_CONTROL_AE_AVAILABLE_MODES, ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
       ANDROID_CONTROL_AE_COMPENSATION_RANGE, ANDROID_CONTROL_AE_COMPENSATION_STEP,
       ANDROID_CONTROL_AF_AVAILABLE_MODES, ANDROID_CONTROL_AVAILABLE_EFFECTS,
       ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
       ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
       ANDROID_CONTROL_AWB_AVAILABLE_MODES, ANDROID_CONTROL_MAX_REGIONS,
       ANDROID_CONTROL_SCENE_MODE_OVERRIDES,ANDROID_FLASH_INFO_AVAILABLE,
       ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES,
       ANDROID_FLASH_INFO_CHARGE_DURATION, ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
       ANDROID_JPEG_MAX_SIZE, ANDROID_LENS_INFO_AVAILABLE_APERTURES,
       ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
       ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
       ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
       ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE, ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
       ANDROID_LENS_INFO_SHADING_MAP_SIZE, ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,
       ANDROID_LENS_FACING, ANDROID_LENS_OPTICAL_AXIS_ANGLE,ANDROID_LENS_POSITION,
       ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS, ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS,
       ANDROID_REQUEST_PIPELINE_MAX_DEPTH, ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
       ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS, ANDROID_REQUEST_AVAILABLE_RESULT_KEYS,
       ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS, ANDROID_REQUEST_PARTIAL_RESULT_COUNT,
       ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
       ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP,
       ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
       ANDROID_SCALER_CROPPING_TYPE,
       /*ANDROID_SCALER_AVAILABLE_STALL_DURATIONS,*/
       ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS, ANDROID_SENSOR_FORWARD_MATRIX1,
       ANDROID_SENSOR_REFERENCE_ILLUMINANT1, ANDROID_SENSOR_REFERENCE_ILLUMINANT2,
       ANDROID_SENSOR_FORWARD_MATRIX2, ANDROID_SENSOR_COLOR_TRANSFORM1,
       ANDROID_SENSOR_COLOR_TRANSFORM2, ANDROID_SENSOR_CALIBRATION_TRANSFORM1,
       ANDROID_SENSOR_CALIBRATION_TRANSFORM2, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
       ANDROID_SENSOR_INFO_SENSITIVITY_RANGE, ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
       ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE, ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
       ANDROID_SENSOR_INFO_PHYSICAL_SIZE, ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE,
       ANDROID_SENSOR_INFO_WHITE_LEVEL, ANDROID_SENSOR_BASE_GAIN_FACTOR,
       ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE,
       ANDROID_SENSOR_BLACK_LEVEL_PATTERN, ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY,
       ANDROID_SENSOR_ORIENTATION, ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES,
       ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
       ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
       ANDROID_STATISTICS_INFO_MAX_FACE_COUNT, ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
       ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
       ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE, ANDROID_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES,
       ANDROID_EDGE_AVAILABLE_EDGE_MODES,
       ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES,
       ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES,
       ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES,
       ANDROID_TONEMAP_MAX_CURVE_POINTS, ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
       ANDROID_SYNC_MAX_LATENCY };
    staticInfo.update(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS,
                      available_characteristics_keys,
                      sizeof(available_characteristics_keys)/sizeof(int32_t));

    /*available stall durations depend on the hw + sw and will be different for different devices */
    /*have to add for raw after implementation*/
    int32_t stall_formats[] = {HAL_PIXEL_FORMAT_BLOB, ANDROID_SCALER_AVAILABLE_FORMATS_RAW16};
    size_t stall_formats_count = sizeof(stall_formats)/sizeof(int32_t);

    size_t available_stall_size = gCamCapability[cameraId]->picture_sizes_tbl_cnt * 4;
    int64_t available_stall_durations[available_stall_size];
    idx = 0;
    for (uint32_t j = 0; j < stall_formats_count; j++) {
       if (stall_formats[j] == HAL_PIXEL_FORMAT_BLOB) {
          for (uint32_t i = 0; i < gCamCapability[cameraId]->picture_sizes_tbl_cnt; i++) {
             available_stall_durations[idx]   = stall_formats[j];
             available_stall_durations[idx+1] = gCamCapability[cameraId]->picture_sizes_tbl[i].width;
             available_stall_durations[idx+2] = gCamCapability[cameraId]->picture_sizes_tbl[i].height;
             available_stall_durations[idx+3] = gCamCapability[cameraId]->jpeg_stall_durations[i];
             idx+=4;
          }
       } else {
          for (uint32_t i = 0; i < gCamCapability[cameraId]->supported_raw_dim_cnt; i++) {
             available_stall_durations[idx]   = stall_formats[j];
             available_stall_durations[idx+1] = gCamCapability[cameraId]->raw_dim[i].width;
             available_stall_durations[idx+2] = gCamCapability[cameraId]->raw_dim[i].height;
             available_stall_durations[idx+3] = gCamCapability[cameraId]->raw16_stall_durations[i];
             idx+=4;
          }
       }
    }
    staticInfo.update(ANDROID_SCALER_AVAILABLE_STALL_DURATIONS,
                      available_stall_durations,
                      idx);

    uint8_t available_correction_modes[] =
        {ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF};
    staticInfo.update(
        ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES,
        available_correction_modes,
        1);

    uint8_t sensor_timestamp_source[] =
        {ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_UNKNOWN};
    staticInfo.update(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE,
                      sensor_timestamp_source,
                      1);

    //QCAMERA3_OPAQUE_RAW
    uint8_t raw_format = QCAMERA3_OPAQUE_RAW_FORMAT_LEGACY;
    cam_format_t fmt = CAM_FORMAT_BAYER_QCOM_RAW_10BPP_GBRG;
    switch (gCamCapability[cameraId]->opaque_raw_fmt) {
    case LEGACY_RAW:
        if (gCamCapability[cameraId]->white_level == (1<<8)-1)
            fmt = CAM_FORMAT_BAYER_QCOM_RAW_8BPP_GBRG;
        else if (gCamCapability[cameraId]->white_level == (1<<10)-1)
            fmt = CAM_FORMAT_BAYER_QCOM_RAW_10BPP_GBRG;
        else if (gCamCapability[cameraId]->white_level == (1<<12)-1)
            fmt = CAM_FORMAT_BAYER_QCOM_RAW_12BPP_GBRG;
        raw_format = QCAMERA3_OPAQUE_RAW_FORMAT_LEGACY;
        break;
    case MIPI_RAW:
        if (gCamCapability[cameraId]->white_level == (1<<8)-1)
            fmt = CAM_FORMAT_BAYER_MIPI_RAW_8BPP_GBRG;
        else if (gCamCapability[cameraId]->white_level == (1<<10)-1)
            fmt = CAM_FORMAT_BAYER_MIPI_RAW_10BPP_GBRG;
        else if (gCamCapability[cameraId]->white_level == (1<<12)-1)
            fmt = CAM_FORMAT_BAYER_MIPI_RAW_12BPP_GBRG;
        raw_format = QCAMERA3_OPAQUE_RAW_FORMAT_MIPI;
        break;
    default:
        ALOGE("%s: unknown opaque_raw_format %d", __func__,
                gCamCapability[cameraId]->opaque_raw_fmt);
        break;
    }
    staticInfo.update(QCAMERA3_OPAQUE_RAW_FORMAT, &raw_format, 1);

    int32_t strides[3*gCamCapability[cameraId]->supported_raw_dim_cnt];
    for (size_t i = 0; i < gCamCapability[cameraId]->supported_raw_dim_cnt; i++) {
        cam_stream_buf_plane_info_t buf_planes;
        strides[i*3] = gCamCapability[cameraId]->raw_dim[i].width;
        strides[i*3+1] = gCamCapability[cameraId]->raw_dim[i].height;
        mm_stream_calc_offset_raw(fmt, &gCamCapability[cameraId]->raw_dim[i],
            &gCamCapability[cameraId]->padding_info, &buf_planes);
        strides[i*3+2] = buf_planes.plane_info.mp[0].stride;
    }
    staticInfo.update(QCAMERA3_OPAQUE_RAW_STRIDES, strides,
            3*gCamCapability[cameraId]->supported_raw_dim_cnt);

    gStaticMetadata[cameraId] = staticInfo.release();
    return rc;
}

/*===========================================================================
 * FUNCTION   : makeTable
 *
 * DESCRIPTION: make a table of sizes
 *
 * PARAMETERS :
 *
 *
 *==========================================================================*/
void QCamera3HardwareInterface::makeTable(cam_dimension_t* dimTable, uint8_t size,
                                          int32_t* sizeTable)
{
    int j = 0;
    for (int i = 0; i < size; i++) {
        sizeTable[j] = dimTable[i].width;
        sizeTable[j+1] = dimTable[i].height;
        j+=2;
    }
}

/*===========================================================================
 * FUNCTION   : makeFPSTable
 *
 * DESCRIPTION: make a table of fps ranges
 *
 * PARAMETERS :
 *
 *==========================================================================*/
void QCamera3HardwareInterface::makeFPSTable(cam_fps_range_t* fpsTable, uint8_t size,
                                          int32_t* fpsRangesTable)
{
    int j = 0;
    for (int i = 0; i < size; i++) {
        fpsRangesTable[j] = (int32_t)fpsTable[i].min_fps;
        fpsRangesTable[j+1] = (int32_t)fpsTable[i].max_fps;
        j+=2;
    }
}

/*===========================================================================
 * FUNCTION   : makeOverridesList
 *
 * DESCRIPTION: make a list of scene mode overrides
 *
 * PARAMETERS :
 *
 *
 *==========================================================================*/
void QCamera3HardwareInterface::makeOverridesList(cam_scene_mode_overrides_t* overridesTable,
                                                  uint8_t size, uint8_t* overridesList,
                                                  uint8_t* supported_indexes,
                                                  int camera_id)
{
    /*daemon will give a list of overrides for all scene modes.
      However we should send the fwk only the overrides for the scene modes
      supported by the framework*/
    int j = 0, index = 0, supt = 0;
    uint8_t focus_override;
    for (int i = 0; i < size; i++) {
        supt = 0;
        index = supported_indexes[i];
        overridesList[j] = gCamCapability[camera_id]->flash_available ? ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH:ANDROID_CONTROL_AE_MODE_ON;
        overridesList[j+1] = (uint8_t)lookupFwkName(WHITE_BALANCE_MODES_MAP,
                                 sizeof(WHITE_BALANCE_MODES_MAP)/sizeof(WHITE_BALANCE_MODES_MAP[0]),
                                                    overridesTable[index].awb_mode);
        focus_override = (uint8_t)overridesTable[index].af_mode;
        for (int k = 0; k < gCamCapability[camera_id]->supported_focus_modes_cnt; k++) {
           if (gCamCapability[camera_id]->supported_focus_modes[k] == focus_override) {
              supt = 1;
              break;
           }
        }
        if (supt) {
           overridesList[j+2] = (uint8_t)lookupFwkName(FOCUS_MODES_MAP,
                                              sizeof(FOCUS_MODES_MAP)/sizeof(FOCUS_MODES_MAP[0]),
                                              focus_override);
        } else {
           overridesList[j+2] = ANDROID_CONTROL_AF_MODE_OFF;
        }
        j+=3;
    }
}

/*===========================================================================
 * FUNCTION   : getPreviewHalPixelFormat
 *
 * DESCRIPTION: convert the format to type recognized by framework
 *
 * PARAMETERS : format : the format from backend
 *
 ** RETURN    : format recognized by framework
 *
 *==========================================================================*/
int32_t QCamera3HardwareInterface::getScalarFormat(int32_t format)
{
    int32_t halPixelFormat;

    switch (format) {
    case CAM_FORMAT_YUV_420_NV12:
        halPixelFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        break;
    case CAM_FORMAT_YUV_420_NV21:
        halPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    case CAM_FORMAT_YUV_420_NV21_ADRENO:
        halPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO;
        break;
    case CAM_FORMAT_YUV_420_YV12:
        halPixelFormat = HAL_PIXEL_FORMAT_YV12;
        break;
    case CAM_FORMAT_YUV_422_NV16:
    case CAM_FORMAT_YUV_422_NV61:
    default:
        halPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    }
    return halPixelFormat;
}
/*===========================================================================
 * FUNCTION   : computeNoiseModelEntryS
 *
 * DESCRIPTION: function to map a given sensitivity to the S noise
 *              model parameters in the DNG noise model.
 *
 * PARAMETERS : sens : the sensor sensitivity
 *
 ** RETURN    : S (sensor amplification) noise
 *
 *==========================================================================*/

double QCamera3HardwareInterface::computeNoiseModelEntryS(int32_t sens) {
   double s = 1.693069e-06 * sens + 3.480007e-05;
   return s < 0.0 ? 0.0 : s;
}

/*===========================================================================
 * FUNCTION   : computeNoiseModelEntryO
 *
 * DESCRIPTION: function to map a given sensitivity to the O noise
 *              model parameters in the DNG noise model.
 *
 * PARAMETERS : sens : the sensor sensitivity
 *
 ** RETURN    : O (sensor readout) noise
 *
 *==========================================================================*/

double QCamera3HardwareInterface::computeNoiseModelEntryO(int32_t sens) {
   double o = 1.301416e-07 * sens + -2.262256e-04;
   return o < 0.0 ? 0.0 : o
;}

/*===========================================================================
 * FUNCTION   : getSensorSensitivity
 *
 * DESCRIPTION: convert iso_mode to an integer value
 *
 * PARAMETERS : iso_mode : the iso_mode supported by sensor
 *
 ** RETURN    : sensitivity supported by sensor
 *
 *==========================================================================*/
int32_t QCamera3HardwareInterface::getSensorSensitivity(int32_t iso_mode)
{
    int32_t sensitivity;

    switch (iso_mode) {
    case CAM_ISO_MODE_100:
        sensitivity = 100;
        break;
    case CAM_ISO_MODE_200:
        sensitivity = 200;
        break;
    case CAM_ISO_MODE_400:
        sensitivity = 400;
        break;
    case CAM_ISO_MODE_800:
        sensitivity = 800;
        break;
    case CAM_ISO_MODE_1600:
        sensitivity = 1600;
        break;
    default:
        sensitivity = -1;
        break;
    }
    return sensitivity;
}

/*===========================================================================
 * FUNCTION   : AddSetMetaEntryToBatch
 *
 * DESCRIPTION: add set parameter entry into batch
 *
 * PARAMETERS :
 *   @p_table     : ptr to parameter buffer
 *   @paramType   : parameter type
 *   @paramLength : length of parameter value
 *   @paramValue  : ptr to parameter value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3HardwareInterface::AddSetMetaEntryToBatch(metadata_buffer_t *p_table,
                                                          unsigned int paramType,
                                                          uint32_t paramLength,
                                                          void *paramValue)
{
    int position = paramType;
    int current, next;

    /*************************************************************************
    *                 Code to take care of linking next flags                *
    *************************************************************************/
    current = GET_FIRST_PARAM_ID(p_table);
    if (position == current){
        //DO NOTHING
    } else if (position < current){
        SET_NEXT_PARAM_ID(position, p_table, current);
        SET_FIRST_PARAM_ID(p_table, position);
    } else {
        /* Search for the position in the linked list where we need to slot in*/
        while (position > GET_NEXT_PARAM_ID(current, p_table))
            current = GET_NEXT_PARAM_ID(current, p_table);

        /*If node already exists no need to alter linking*/
        if (position != GET_NEXT_PARAM_ID(current, p_table)) {
            next = GET_NEXT_PARAM_ID(current, p_table);
            SET_NEXT_PARAM_ID(current, p_table, position);
            SET_NEXT_PARAM_ID(position, p_table, next);
        }
    }

    /*************************************************************************
    *                   Copy contents into entry                             *
    *************************************************************************/

    if (paramLength > sizeof(parm_type_t)) {
        ALOGE("%s:Size of input larger than max entry size",__func__);
        return BAD_VALUE;
    }
    memcpy(POINTER_OF(paramType,p_table), paramValue, paramLength);
    SET_PARM_VALID_BIT(paramType,p_table,1);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : lookupFwkName
 *
 * DESCRIPTION: In case the enum is not same in fwk and backend
 *              make sure the parameter is correctly propogated
 *
 * PARAMETERS  :
 *   @arr      : map between the two enums
 *   @len      : len of the map
 *   @hal_name : name of the hal_parm to map
 *
 * RETURN     : int type of status
 *              fwk_name  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3HardwareInterface::lookupFwkName(const QCameraMap arr[],
                                             int len, int hal_name)
{

    for (int i = 0; i < len; i++) {
        if (arr[i].hal_name == hal_name)
            return arr[i].fwk_name;
    }

    /* Not able to find matching framework type is not necessarily
     * an error case. This happens when mm-camera supports more attributes
     * than the frameworks do */
    ALOGD("%s: Cannot find matching framework type", __func__);
    return NAME_NOT_FOUND;
}

/*===========================================================================
 * FUNCTION   : lookupHalName
 *
 * DESCRIPTION: In case the enum is not same in fwk and backend
 *              make sure the parameter is correctly propogated
 *
 * PARAMETERS  :
 *   @arr      : map between the two enums
 *   @len      : len of the map
 *   @fwk_name : name of the hal_parm to map
 *
 * RETURN     : int32_t type of status
 *              hal_name  -- success
 *              none-zero failure code
 *==========================================================================*/
int8_t QCamera3HardwareInterface::lookupHalName(const QCameraMap arr[],
                                             int len, unsigned int fwk_name)
{
    for (int i = 0; i < len; i++) {
       if (arr[i].fwk_name == fwk_name)
           return arr[i].hal_name;
    }
    ALOGE("%s: Cannot find matching hal type", __func__);
    return NAME_NOT_FOUND;
}

/*===========================================================================
 * FUNCTION   : getCapabilities
 *
 * DESCRIPTION: query camera capabilities
 *
 * PARAMETERS :
 *   @cameraId  : camera Id
 *   @info      : camera info struct to be filled in with camera capabilities
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::getCamInfo(int cameraId,
                                    struct camera_info *info)
{
    int rc = 0;

    if (NULL == gCamCapability[cameraId]) {
        rc = initCapabilities(cameraId);
        if (rc < 0) {
            //pthread_mutex_unlock(&g_camlock);
            return rc;
        }
    }

    if (NULL == gStaticMetadata[cameraId]) {
        rc = initStaticMetadata(cameraId);
        if (rc < 0) {
            return rc;
        }
    }

    switch(gCamCapability[cameraId]->position) {
    case CAM_POSITION_BACK:
        info->facing = CAMERA_FACING_BACK;
        break;

    case CAM_POSITION_FRONT:
        info->facing = CAMERA_FACING_FRONT;
        break;

    default:
        ALOGE("%s:Unknown position type for camera id:%d", __func__, cameraId);
        rc = -1;
        break;
    }


    info->orientation = gCamCapability[cameraId]->sensor_mount_angle;
    info->device_version = CAMERA_DEVICE_API_VERSION_3_2;
    info->static_camera_characteristics = gStaticMetadata[cameraId];

    return rc;
}

/*===========================================================================
 * FUNCTION   : translateCapabilityToMetadata
 *
 * DESCRIPTION: translate the capability into camera_metadata_t
 *
 * PARAMETERS : type of the request
 *
 *
 * RETURN     : success: camera_metadata_t*
 *              failure: NULL
 *
 *==========================================================================*/
camera_metadata_t* QCamera3HardwareInterface::translateCapabilityToMetadata(int type)
{
    pthread_mutex_lock(&mMutex);

    if (mDefaultMetadata[type] != NULL) {
        pthread_mutex_unlock(&mMutex);
        return mDefaultMetadata[type];
    }
    //first time we are handling this request
    //fill up the metadata structure using the wrapper class
    CameraMetadata settings;
    //translate from cam_capability_t to camera_metadata_tag_t
    static const uint8_t requestType = ANDROID_REQUEST_TYPE_CAPTURE;
    settings.update(ANDROID_REQUEST_TYPE, &requestType, 1);
    int32_t defaultRequestID = 0;
    settings.update(ANDROID_REQUEST_ID, &defaultRequestID, 1);

    uint8_t controlIntent = 0;
    uint8_t focusMode;
    switch (type) {
      case CAMERA3_TEMPLATE_PREVIEW:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
        focusMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        break;
      case CAMERA3_TEMPLATE_STILL_CAPTURE:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
        focusMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        break;
      case CAMERA3_TEMPLATE_VIDEO_RECORD:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
        focusMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        break;
      case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
        focusMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        break;
      case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG;
        focusMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        break;
      case CAMERA3_TEMPLATE_MANUAL:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_MANUAL;
        focusMode = ANDROID_CONTROL_AF_MODE_OFF;
        break;
      default:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
        break;
    }
    settings.update(ANDROID_CONTROL_CAPTURE_INTENT, &controlIntent, 1);

    if (gCamCapability[mCameraId]->supported_focus_modes_cnt == 1) {
        focusMode = ANDROID_CONTROL_AF_MODE_OFF;
    }
    settings.update(ANDROID_CONTROL_AF_MODE, &focusMode, 1);

    settings.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
            &gCamCapability[mCameraId]->exposure_compensation_default, 1);

    static const uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_OFF;
    settings.update(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);

    static const uint8_t awbLock = ANDROID_CONTROL_AWB_LOCK_OFF;
    settings.update(ANDROID_CONTROL_AWB_LOCK, &awbLock, 1);

    static const uint8_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    settings.update(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);

    static const uint8_t controlMode = ANDROID_CONTROL_MODE_AUTO;
    settings.update(ANDROID_CONTROL_MODE, &controlMode, 1);

    static const uint8_t effectMode = ANDROID_CONTROL_EFFECT_MODE_OFF;
    settings.update(ANDROID_CONTROL_EFFECT_MODE, &effectMode, 1);

    static const uint8_t sceneMode = ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY;
    settings.update(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);

    static const uint8_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    settings.update(ANDROID_CONTROL_AE_MODE, &aeMode, 1);

    /*flash*/
    static const uint8_t flashMode = ANDROID_FLASH_MODE_OFF;
    settings.update(ANDROID_FLASH_MODE, &flashMode, 1);

    static const uint8_t flashFiringLevel = CAM_FLASH_FIRING_LEVEL_4;
    settings.update(ANDROID_FLASH_FIRING_POWER,
            &flashFiringLevel, 1);

    /* lens */
    float default_aperture = gCamCapability[mCameraId]->apertures[0];
    settings.update(ANDROID_LENS_APERTURE, &default_aperture, 1);

    if (gCamCapability[mCameraId]->filter_densities_count) {
        float default_filter_density = gCamCapability[mCameraId]->filter_densities[0];
        settings.update(ANDROID_LENS_FILTER_DENSITY, &default_filter_density,
                        gCamCapability[mCameraId]->filter_densities_count);
    }

    float default_focal_length = gCamCapability[mCameraId]->focal_length;
    settings.update(ANDROID_LENS_FOCAL_LENGTH, &default_focal_length, 1);

    float default_focus_distance = 0;
    settings.update(ANDROID_LENS_FOCUS_DISTANCE, &default_focus_distance, 1);

    static const uint8_t demosaicMode = ANDROID_DEMOSAIC_MODE_FAST;
    settings.update(ANDROID_DEMOSAIC_MODE, &demosaicMode, 1);

    static const uint8_t hotpixelMode = ANDROID_HOT_PIXEL_MODE_FAST;
    settings.update(ANDROID_HOT_PIXEL_MODE, &hotpixelMode, 1);

    static const int32_t testpatternMode = ANDROID_SENSOR_TEST_PATTERN_MODE_OFF;
    settings.update(ANDROID_SENSOR_TEST_PATTERN_MODE, &testpatternMode, 1);

    static const uint8_t faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
    settings.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, 1);

    static const uint8_t histogramMode = ANDROID_STATISTICS_HISTOGRAM_MODE_OFF;
    settings.update(ANDROID_STATISTICS_HISTOGRAM_MODE, &histogramMode, 1);

    static const uint8_t sharpnessMapMode = ANDROID_STATISTICS_SHARPNESS_MAP_MODE_OFF;
    settings.update(ANDROID_STATISTICS_SHARPNESS_MAP_MODE, &sharpnessMapMode, 1);

    static const uint8_t hotPixelMapMode = ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_OFF;
    settings.update(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE, &hotPixelMapMode, 1);

    /* Lens shading map mode */
    uint8_t shadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF;
    if (type == CAMERA3_TEMPLATE_STILL_CAPTURE &&
        gCamCapability[mCameraId]->supported_raw_dim_cnt) {
      shadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_ON;
    }
    settings.update(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, &shadingMapMode, 1);

    static const uint8_t blackLevelLock = ANDROID_BLACK_LEVEL_LOCK_OFF;
    settings.update(ANDROID_BLACK_LEVEL_LOCK, &blackLevelLock, 1);

    /* Exposure time(Update the Min Exposure Time)*/
    int64_t default_exposure_time = gCamCapability[mCameraId]->exposure_time_range[0];
    settings.update(ANDROID_SENSOR_EXPOSURE_TIME, &default_exposure_time, 1);

    /* frame duration */
    static const int64_t default_frame_duration = NSEC_PER_33MSEC;
    settings.update(ANDROID_SENSOR_FRAME_DURATION, &default_frame_duration, 1);

    /* sensitivity */
    static const int32_t default_sensitivity = 100;
    settings.update(ANDROID_SENSOR_SENSITIVITY, &default_sensitivity, 1);

    /*edge mode*/
    static const uint8_t edge_mode = ANDROID_EDGE_MODE_FAST;
    settings.update(ANDROID_EDGE_MODE, &edge_mode, 1);

    /*noise reduction mode*/
    static const uint8_t noise_red_mode = ANDROID_NOISE_REDUCTION_MODE_FAST;
    settings.update(ANDROID_NOISE_REDUCTION_MODE, &noise_red_mode, 1);

    /*color correction mode*/
    static const uint8_t color_correct_mode = ANDROID_COLOR_CORRECTION_MODE_FAST;
    settings.update(ANDROID_COLOR_CORRECTION_MODE, &color_correct_mode, 1);

    /*transform matrix mode*/
    static const uint8_t tonemap_mode = ANDROID_TONEMAP_MODE_FAST;
    settings.update(ANDROID_TONEMAP_MODE, &tonemap_mode, 1);

    uint8_t edge_strength = (uint8_t)gCamCapability[mCameraId]->sharpness_ctrl.def_value;
    settings.update(ANDROID_EDGE_STRENGTH, &edge_strength, 1);

    int32_t scaler_crop_region[4];
    scaler_crop_region[0] = 0;
    scaler_crop_region[1] = 0;
    scaler_crop_region[2] = gCamCapability[mCameraId]->active_array_size.width;
    scaler_crop_region[3] = gCamCapability[mCameraId]->active_array_size.height;
    settings.update(ANDROID_SCALER_CROP_REGION, scaler_crop_region, 4);

    static const uint8_t antibanding_mode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    settings.update(ANDROID_CONTROL_AE_ANTIBANDING_MODE, &antibanding_mode, 1);

    static const uint8_t vs_mode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
    settings.update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &vs_mode, 1);

    uint8_t opt_stab_mode = (gCamCapability[mCameraId]->optical_stab_modes_count == 2)?
                             ANDROID_LENS_OPTICAL_STABILIZATION_MODE_ON :
                             ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    settings.update(ANDROID_LENS_OPTICAL_STABILIZATION_MODE, &opt_stab_mode, 1);

    /*focus distance*/
    float focus_distance = 0.0;
    settings.update(ANDROID_LENS_FOCUS_DISTANCE, &focus_distance, 1);

    /*target fps range: use maximum range for picture, and maximum fixed range for video*/
    float max_range = 0.0;
    float max_fixed_fps = 0.0;
    int32_t fps_range[2] = {0, 0};
    for (uint32_t i = 0; i < gCamCapability[mCameraId]->fps_ranges_tbl_cnt;
            i++) {
        float range = gCamCapability[mCameraId]->fps_ranges_tbl[i].max_fps -
            gCamCapability[mCameraId]->fps_ranges_tbl[i].min_fps;
        if (type == CAMERA3_TEMPLATE_PREVIEW ||
                type == CAMERA3_TEMPLATE_STILL_CAPTURE ||
                type == CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG) {
            if (range > max_range) {
                fps_range[0] =
                    (int32_t)gCamCapability[mCameraId]->fps_ranges_tbl[i].min_fps;
                fps_range[1] =
                    (int32_t)gCamCapability[mCameraId]->fps_ranges_tbl[i].max_fps;
                max_range = range;
            }
        } else {
            if (range < 0.01 && max_fixed_fps <
                    gCamCapability[mCameraId]->fps_ranges_tbl[i].max_fps) {
                fps_range[0] =
                    (int32_t)gCamCapability[mCameraId]->fps_ranges_tbl[i].min_fps;
                fps_range[1] =
                    (int32_t)gCamCapability[mCameraId]->fps_ranges_tbl[i].max_fps;
                max_fixed_fps = gCamCapability[mCameraId]->fps_ranges_tbl[i].max_fps;
            }
        }
    }
    settings.update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, fps_range, 2);

    /*precapture trigger*/
    uint8_t precapture_trigger = ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
    settings.update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, &precapture_trigger, 1);

    /*af trigger*/
    uint8_t af_trigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
    settings.update(ANDROID_CONTROL_AF_TRIGGER, &af_trigger, 1);

    /* ae & af regions */
    int32_t active_region[] = {
            gCamCapability[mCameraId]->active_array_size.left,
            gCamCapability[mCameraId]->active_array_size.top,
            gCamCapability[mCameraId]->active_array_size.left +
                    gCamCapability[mCameraId]->active_array_size.width,
            gCamCapability[mCameraId]->active_array_size.top +
                    gCamCapability[mCameraId]->active_array_size.height,
            0};
    settings.update(ANDROID_CONTROL_AE_REGIONS, active_region, 5);
    settings.update(ANDROID_CONTROL_AF_REGIONS, active_region, 5);

    /* black level lock */
    uint8_t blacklevel_lock = ANDROID_BLACK_LEVEL_LOCK_OFF;
    settings.update(ANDROID_BLACK_LEVEL_LOCK, &blacklevel_lock, 1);

    //special defaults for manual template
    if (type == CAMERA3_TEMPLATE_MANUAL) {
        static const uint8_t manualControlMode = ANDROID_CONTROL_MODE_OFF;
        settings.update(ANDROID_CONTROL_MODE, &manualControlMode, 1);

        static const uint8_t manualFocusMode = ANDROID_CONTROL_AF_MODE_OFF;
        settings.update(ANDROID_CONTROL_AF_MODE, &manualFocusMode, 1);

        static const uint8_t manualAeMode = ANDROID_CONTROL_AE_MODE_OFF;
        settings.update(ANDROID_CONTROL_AE_MODE, &manualAeMode, 1);

        static const uint8_t manualAwbMode = ANDROID_CONTROL_AWB_MODE_OFF;
        settings.update(ANDROID_CONTROL_AWB_MODE, &manualAwbMode, 1);

        static const uint8_t manualTonemapMode = ANDROID_TONEMAP_MODE_FAST;
        settings.update(ANDROID_TONEMAP_MODE, &manualTonemapMode, 1);

        static const uint8_t manualColorCorrectMode = ANDROID_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX;
        settings.update(ANDROID_COLOR_CORRECTION_MODE, &manualColorCorrectMode, 1);
    }
    mDefaultMetadata[type] = settings.release();

    pthread_mutex_unlock(&mMutex);
    return mDefaultMetadata[type];
}

/*===========================================================================
 * FUNCTION   : setFrameParameters
 *
 * DESCRIPTION: set parameters per frame as requested in the metadata from
 *              framework
 *
 * PARAMETERS :
 *   @request   : request that needs to be serviced
 *   @streamID : Stream ID of all the requested streams
 *
 * RETURN     : success: NO_ERROR
 *              failure:
 *==========================================================================*/
int QCamera3HardwareInterface::setFrameParameters(
                    camera3_capture_request_t *request,
                    cam_stream_ID_t streamID)
{
    /*translate from camera_metadata_t type to parm_type_t*/
    int rc = 0;
    int32_t hal_version = CAM_HAL_V3;
    if (mRepeatingRequest == true) {
       //chain of repeating request
       ALOGV("%s: chain of repeating request", __func__);
    } else {
       memcpy(mPrevParameters, mParameters, sizeof(metadata_buffer_t));
    }

    memset(mParameters, 0, sizeof(metadata_buffer_t));
    mParameters->first_flagged_entry = CAM_INTF_PARM_MAX;
    rc = AddSetMetaEntryToBatch(mParameters, CAM_INTF_PARM_HAL_VERSION,
                sizeof(hal_version), &hal_version);
    if (rc < 0) {
        ALOGE("%s: Failed to set hal version in the parameters", __func__);
        return BAD_VALUE;
    }

    /*we need to update the frame number in the parameters*/
    rc = AddSetMetaEntryToBatch(mParameters, CAM_INTF_META_FRAME_NUMBER,
                                sizeof(request->frame_number), &(request->frame_number));
    if (rc < 0) {
        ALOGE("%s: Failed to set the frame number in the parameters", __func__);
        return BAD_VALUE;
    }

    /* Update stream id of all the requested buffers */
    rc = AddSetMetaEntryToBatch(mParameters, CAM_INTF_META_STREAM_ID,
                                sizeof(cam_stream_ID_t), &streamID);

    if (rc < 0) {
        ALOGE("%s: Failed to set stream type mask in the parameters", __func__);
        return BAD_VALUE;
    }

    if(request->settings != NULL){
        mRepeatingRequest = false;
        rc = translateToHalMetadata(request, mParameters);
    } else {
       mRepeatingRequest = true;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : setReprocParameters
 *
 * DESCRIPTION: Translate frameworks metadata to HAL metadata structure, and
 *              queue it to picture channel for reprocessing.
 *
 * PARAMETERS :
 *   @request   : request that needs to be serviced
 *
 * RETURN     : success: NO_ERROR
 *              failure: non zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::setReprocParameters(
        camera3_capture_request_t *request)
{
    /*translate from camera_metadata_t type to parm_type_t*/
    int rc = 0;
    metadata_buffer_t *reprocParam = NULL;

    if(request->settings != NULL){
        ALOGE("%s: Reprocess settings cannot be NULL", __func__);
        return BAD_VALUE;
    }
    reprocParam = (metadata_buffer_t *)malloc(sizeof(metadata_buffer_t));
    if (!reprocParam) {
        ALOGE("%s: Failed to allocate reprocessing metadata buffer", __func__);
        return NO_MEMORY;
    }
    memset(reprocParam, 0, sizeof(metadata_buffer_t));
    reprocParam->first_flagged_entry = CAM_INTF_PARM_MAX;

    /*we need to update the frame number in the parameters*/
    rc = AddSetMetaEntryToBatch(reprocParam, CAM_INTF_META_FRAME_NUMBER,
                                sizeof(request->frame_number), &(request->frame_number));
    if (rc < 0) {
        ALOGE("%s: Failed to set the frame number in the parameters", __func__);
        return BAD_VALUE;
    }


    rc = translateToHalMetadata(request, reprocParam);
    if (rc < 0) {
        ALOGE("%s: Failed to translate reproc request", __func__);
        delete reprocParam;
        return rc;
    }
    /*queue metadata for reprocessing*/
    rc = mPictureChannel->queueReprocMetadata(reprocParam);
    if (rc < 0) {
        ALOGE("%s: Failed to queue reprocessing metadata", __func__);
        delete reprocParam;
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : translateToHalMetadata
 *
 * DESCRIPTION: read from the camera_metadata_t and change to parm_type_t
 *
 *
 * PARAMETERS :
 *   @request  : request sent from framework
 *
 *
 * RETURN     : success: NO_ERROR
 *              failure:
 *==========================================================================*/
int QCamera3HardwareInterface::translateToHalMetadata
                                  (const camera3_capture_request_t *request,
                                   metadata_buffer_t *hal_metadata)
{
    int rc = 0;
    CameraMetadata frame_settings;
    frame_settings = request->settings;

    /* Do not change the order of the following list unless you know what you are
     * doing.
     * The order is laid out in such a way that parameters in the front of the table
     * may be used to override the parameters later in the table. Examples are:
     * 1. META_MODE should precede AEC/AWB/AF MODE
     * 2. AEC MODE should preced EXPOSURE_TIME/SENSITIVITY/FRAME_DURATION
     * 3. AWB_MODE should precede COLOR_CORRECTION_MODE
     * 4. Any mode should precede it's corresponding settings
     */
    if (frame_settings.exists(ANDROID_CONTROL_MODE)) {
        uint8_t metaMode = frame_settings.find(ANDROID_CONTROL_MODE).data.u8[0];
        rc = AddSetMetaEntryToBatch(mParameters, CAM_INTF_META_MODE,
                sizeof(metaMode), &metaMode);
        if (metaMode == ANDROID_CONTROL_MODE_USE_SCENE_MODE) {
           uint8_t fwk_sceneMode = frame_settings.find(ANDROID_CONTROL_SCENE_MODE).data.u8[0];
           uint8_t sceneMode = lookupHalName(SCENE_MODES_MAP,
                                             sizeof(SCENE_MODES_MAP)/sizeof(SCENE_MODES_MAP[0]),
                                             fwk_sceneMode);
           rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_BESTSHOT_MODE,
                sizeof(sceneMode), &sceneMode);
        } else if (metaMode == ANDROID_CONTROL_MODE_OFF) {
           uint8_t sceneMode = CAM_SCENE_MODE_OFF;
           rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_BESTSHOT_MODE,
                sizeof(sceneMode), &sceneMode);
        } else if (metaMode == ANDROID_CONTROL_MODE_AUTO) {
           uint8_t sceneMode = CAM_SCENE_MODE_OFF;
           rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_BESTSHOT_MODE,
                sizeof(sceneMode), &sceneMode);
        }
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_MODE)) {
        uint8_t fwk_aeMode =
            frame_settings.find(ANDROID_CONTROL_AE_MODE).data.u8[0];
        uint8_t aeMode;
        int32_t redeye;

        if (fwk_aeMode == ANDROID_CONTROL_AE_MODE_OFF ) {
            aeMode = CAM_AE_MODE_OFF;
        } else {
            aeMode = CAM_AE_MODE_ON;
        }
        if (fwk_aeMode == ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE) {
            redeye = 1;
        } else {
            redeye = 0;
        }

        int32_t flashMode = (int32_t)lookupHalName(AE_FLASH_MODE_MAP,
                                          sizeof(AE_FLASH_MODE_MAP),
                                          fwk_aeMode);
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_AEC_MODE,
                sizeof(aeMode), &aeMode);
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_LED_MODE,
                sizeof(flashMode), &flashMode);
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_REDEYE_REDUCTION,
                sizeof(redeye), &redeye);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AWB_MODE)) {
        uint8_t fwk_whiteLevel =
            frame_settings.find(ANDROID_CONTROL_AWB_MODE).data.u8[0];
        uint8_t whiteLevel = lookupHalName(WHITE_BALANCE_MODES_MAP,
                sizeof(WHITE_BALANCE_MODES_MAP),
                fwk_whiteLevel);
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_WHITE_BALANCE,
                sizeof(whiteLevel), &whiteLevel);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AF_MODE)) {
        uint8_t fwk_focusMode =
            frame_settings.find(ANDROID_CONTROL_AF_MODE).data.u8[0];
        uint8_t focusMode;
        focusMode = lookupHalName(FOCUS_MODES_MAP,
                                   sizeof(FOCUS_MODES_MAP),
                                   fwk_focusMode);
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_FOCUS_MODE,
                sizeof(focusMode), &focusMode);
    }

    if (frame_settings.exists(ANDROID_LENS_FOCUS_DISTANCE)) {
        float focalDistance = frame_settings.find(ANDROID_LENS_FOCUS_DISTANCE).data.f[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_LENS_FOCUS_DISTANCE,
                sizeof(focalDistance), &focalDistance);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_ANTIBANDING_MODE)) {
        uint8_t fwk_antibandingMode =
            frame_settings.find(ANDROID_CONTROL_AE_ANTIBANDING_MODE).data.u8[0];
        uint8_t hal_antibandingMode = lookupHalName(ANTIBANDING_MODES_MAP,
                     sizeof(ANTIBANDING_MODES_MAP)/sizeof(ANTIBANDING_MODES_MAP[0]),
                     fwk_antibandingMode);
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_ANTIBANDING,
                sizeof(hal_antibandingMode), &hal_antibandingMode);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION)) {
        int32_t expCompensation = frame_settings.find(
            ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION).data.i32[0];
        if (expCompensation < gCamCapability[mCameraId]->exposure_compensation_min)
            expCompensation = gCamCapability[mCameraId]->exposure_compensation_min;
        if (expCompensation > gCamCapability[mCameraId]->exposure_compensation_max)
            expCompensation = gCamCapability[mCameraId]->exposure_compensation_max;
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_EV,
          sizeof(expCompensation), &expCompensation);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION)) {
        int32_t expCompensation = frame_settings.find(
            ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION).data.i32[0];
        if (expCompensation < gCamCapability[mCameraId]->exposure_compensation_min)
            expCompensation = gCamCapability[mCameraId]->exposure_compensation_min;
        if (expCompensation > gCamCapability[mCameraId]->exposure_compensation_max)
            expCompensation = gCamCapability[mCameraId]->exposure_compensation_max;
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_EV,
          sizeof(expCompensation), &expCompensation);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_LOCK)) {
        uint8_t aeLock = frame_settings.find(ANDROID_CONTROL_AE_LOCK).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_AEC_LOCK,
                sizeof(aeLock), &aeLock);
    }
    if (frame_settings.exists(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)) {
        cam_fps_range_t fps_range;
        fps_range.min_fps =
            frame_settings.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE).data.i32[0];
        fps_range.max_fps =
            frame_settings.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE).data.i32[1];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_FPS_RANGE,
                sizeof(fps_range), &fps_range);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AWB_LOCK)) {
        uint8_t awbLock =
            frame_settings.find(ANDROID_CONTROL_AWB_LOCK).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_AWB_LOCK,
                sizeof(awbLock), &awbLock);
    }

    if (frame_settings.exists(ANDROID_CONTROL_EFFECT_MODE)) {
        uint8_t fwk_effectMode =
            frame_settings.find(ANDROID_CONTROL_EFFECT_MODE).data.u8[0];
        uint8_t effectMode = lookupHalName(EFFECT_MODES_MAP,
                sizeof(EFFECT_MODES_MAP),
                fwk_effectMode);
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_EFFECT,
                sizeof(effectMode), &effectMode);
    }

    if (frame_settings.exists(ANDROID_COLOR_CORRECTION_MODE)) {
        uint8_t colorCorrectMode =
            frame_settings.find(ANDROID_COLOR_CORRECTION_MODE).data.u8[0];
        rc =
            AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_COLOR_CORRECT_MODE,
                    sizeof(colorCorrectMode), &colorCorrectMode);
    }

    if (frame_settings.exists(ANDROID_COLOR_CORRECTION_GAINS)) {
        cam_color_correct_gains_t colorCorrectGains;
        for (int i = 0; i < 4; i++) {
            colorCorrectGains.gains[i] =
                frame_settings.find(ANDROID_COLOR_CORRECTION_GAINS).data.f[i];
        }
        rc =
            AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_COLOR_CORRECT_GAINS,
                    sizeof(colorCorrectGains), &colorCorrectGains);
    }

    if (frame_settings.exists(ANDROID_COLOR_CORRECTION_TRANSFORM)) {
        cam_color_correct_matrix_t colorCorrectTransform;
        cam_rational_type_t transform_elem;
        int num = 0;
        for (int i = 0; i < 3; i++) {
           for (int j = 0; j < 3; j++) {
              transform_elem.numerator =
                 frame_settings.find(ANDROID_COLOR_CORRECTION_TRANSFORM).data.r[num].numerator;
              transform_elem.denominator =
                 frame_settings.find(ANDROID_COLOR_CORRECTION_TRANSFORM).data.r[num].denominator;
              colorCorrectTransform.transform_matrix[i][j] = transform_elem;
              num++;
           }
        }
        rc =
            AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_COLOR_CORRECT_TRANSFORM,
                    sizeof(colorCorrectTransform), &colorCorrectTransform);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER)) {
        cam_trigger_t aecTrigger;
        aecTrigger.trigger =
            frame_settings.find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_AEC_PRECAPTURE_TRIGGER,
                sizeof(aecTrigger), &aecTrigger);
    }

    /*af_trigger must come with a trigger id*/
    if (frame_settings.exists(ANDROID_CONTROL_AF_TRIGGER)) {
        cam_trigger_t af_trigger;
        af_trigger.trigger =
            frame_settings.find(ANDROID_CONTROL_AF_TRIGGER).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_AF_TRIGGER, sizeof(af_trigger), &af_trigger);
    }

    if (frame_settings.exists(ANDROID_DEMOSAIC_MODE)) {
        int32_t demosaic =
            frame_settings.find(ANDROID_DEMOSAIC_MODE).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_DEMOSAIC,
                sizeof(demosaic), &demosaic);
    }

    if (frame_settings.exists(ANDROID_EDGE_MODE)) {
        cam_edge_application_t edge_application;
        edge_application.edge_mode = frame_settings.find(ANDROID_EDGE_MODE).data.u8[0];
        if (edge_application.edge_mode == CAM_EDGE_MODE_OFF) {
            edge_application.sharpness = 0;
        } else {
            if (frame_settings.exists(ANDROID_EDGE_STRENGTH)) {
                uint8_t edgeStrength =
                    frame_settings.find(ANDROID_EDGE_STRENGTH).data.u8[0];
                edge_application.sharpness = (int32_t)edgeStrength;
            } else {
                edge_application.sharpness = gCamCapability[mCameraId]->sharpness_ctrl.def_value; //default
            }
        }
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_EDGE_MODE,
                sizeof(edge_application), &edge_application);
    }

    if (frame_settings.exists(ANDROID_FLASH_MODE)) {
        int32_t respectFlashMode = 1;
        if (frame_settings.exists(ANDROID_CONTROL_AE_MODE)) {
            uint8_t fwk_aeMode =
                frame_settings.find(ANDROID_CONTROL_AE_MODE).data.u8[0];
            if (fwk_aeMode > ANDROID_CONTROL_AE_MODE_ON) {
                respectFlashMode = 0;
                ALOGV("%s: AE Mode controls flash, ignore android.flash.mode",
                    __func__);
            }
        }
        if (respectFlashMode) {
            uint8_t flashMode =
                frame_settings.find(ANDROID_FLASH_MODE).data.u8[0];
            flashMode = (int32_t)lookupHalName(FLASH_MODES_MAP,
                                          sizeof(FLASH_MODES_MAP),
                                          flashMode);
            ALOGV("%s: flash mode after mapping %d", __func__, flashMode);
            // To check: CAM_INTF_META_FLASH_MODE usage
            rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_LED_MODE,
                          sizeof(flashMode), &flashMode);
        }
    }

    if (frame_settings.exists(ANDROID_FLASH_FIRING_POWER)) {
        uint8_t flashPower =
            frame_settings.find(ANDROID_FLASH_FIRING_POWER).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_FLASH_POWER,
                sizeof(flashPower), &flashPower);
    }

    if (frame_settings.exists(ANDROID_FLASH_FIRING_TIME)) {
        int64_t flashFiringTime =
            frame_settings.find(ANDROID_FLASH_FIRING_TIME).data.i64[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_FLASH_FIRING_TIME, sizeof(flashFiringTime), &flashFiringTime);
    }

    if (frame_settings.exists(ANDROID_HOT_PIXEL_MODE)) {
        uint8_t hotPixelMode =
            frame_settings.find(ANDROID_HOT_PIXEL_MODE).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_HOTPIXEL_MODE,
                sizeof(hotPixelMode), &hotPixelMode);
    }

    if (frame_settings.exists(ANDROID_LENS_APERTURE)) {
        float lensAperture =
            frame_settings.find( ANDROID_LENS_APERTURE).data.f[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_LENS_APERTURE,
                sizeof(lensAperture), &lensAperture);
    }

    if (frame_settings.exists(ANDROID_LENS_FILTER_DENSITY)) {
        float filterDensity =
            frame_settings.find(ANDROID_LENS_FILTER_DENSITY).data.f[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_LENS_FILTERDENSITY,
                sizeof(filterDensity), &filterDensity);
    }

    if (frame_settings.exists(ANDROID_LENS_FOCAL_LENGTH)) {
        float focalLength =
            frame_settings.find(ANDROID_LENS_FOCAL_LENGTH).data.f[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_LENS_FOCAL_LENGTH,
                sizeof(focalLength), &focalLength);
    }

    if (frame_settings.exists(ANDROID_LENS_OPTICAL_STABILIZATION_MODE)) {
        uint8_t optStabMode =
            frame_settings.find(ANDROID_LENS_OPTICAL_STABILIZATION_MODE).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_LENS_OPT_STAB_MODE,
                sizeof(optStabMode), &optStabMode);
    }

    if (frame_settings.exists(ANDROID_NOISE_REDUCTION_MODE)) {
        uint8_t noiseRedMode =
            frame_settings.find(ANDROID_NOISE_REDUCTION_MODE).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_NOISE_REDUCTION_MODE,
                sizeof(noiseRedMode), &noiseRedMode);
    }

    if (frame_settings.exists(ANDROID_NOISE_REDUCTION_STRENGTH)) {
        uint8_t noiseRedStrength =
            frame_settings.find(ANDROID_NOISE_REDUCTION_STRENGTH).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_NOISE_REDUCTION_STRENGTH,
                sizeof(noiseRedStrength), &noiseRedStrength);
    }

    cam_crop_region_t scalerCropRegion;
    bool scalerCropSet = false;
    if (frame_settings.exists(ANDROID_SCALER_CROP_REGION)) {
        scalerCropRegion.left =
            frame_settings.find(ANDROID_SCALER_CROP_REGION).data.i32[0];
        scalerCropRegion.top =
            frame_settings.find(ANDROID_SCALER_CROP_REGION).data.i32[1];
        scalerCropRegion.width =
            frame_settings.find(ANDROID_SCALER_CROP_REGION).data.i32[2];
        scalerCropRegion.height =
            frame_settings.find(ANDROID_SCALER_CROP_REGION).data.i32[3];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_SCALER_CROP_REGION,
                sizeof(scalerCropRegion), &scalerCropRegion);
        scalerCropSet = true;
    }

    if (frame_settings.exists(ANDROID_SENSOR_EXPOSURE_TIME)) {
        int64_t sensorExpTime =
            frame_settings.find(ANDROID_SENSOR_EXPOSURE_TIME).data.i64[0];
        ALOGV("%s: setting sensorExpTime %lld", __func__, sensorExpTime);
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_SENSOR_EXPOSURE_TIME,
                sizeof(sensorExpTime), &sensorExpTime);
    }

    if (frame_settings.exists(ANDROID_SENSOR_FRAME_DURATION)) {
        int64_t sensorFrameDuration =
            frame_settings.find(ANDROID_SENSOR_FRAME_DURATION).data.i64[0];
        int64_t minFrameDuration = getMinFrameDuration(request);
        sensorFrameDuration = MAX(sensorFrameDuration, minFrameDuration);
        if (sensorFrameDuration > gCamCapability[mCameraId]->max_frame_duration)
            sensorFrameDuration = gCamCapability[mCameraId]->max_frame_duration;
        ALOGV("%s: clamp sensorFrameDuration to %lld", __func__, sensorFrameDuration);
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_SENSOR_FRAME_DURATION,
                sizeof(sensorFrameDuration), &sensorFrameDuration);
    }

    if (frame_settings.exists(ANDROID_SENSOR_SENSITIVITY)) {
        int32_t sensorSensitivity =
            frame_settings.find(ANDROID_SENSOR_SENSITIVITY).data.i32[0];
        if (sensorSensitivity <
                gCamCapability[mCameraId]->sensitivity_range.min_sensitivity)
            sensorSensitivity =
                gCamCapability[mCameraId]->sensitivity_range.min_sensitivity;
        if (sensorSensitivity >
                gCamCapability[mCameraId]->sensitivity_range.max_sensitivity)
            sensorSensitivity =
                gCamCapability[mCameraId]->sensitivity_range.max_sensitivity;
        ALOGV("%s: clamp sensorSensitivity to %d", __func__, sensorSensitivity);
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_SENSOR_SENSITIVITY,
                sizeof(sensorSensitivity), &sensorSensitivity);
    }

    if (frame_settings.exists(ANDROID_SHADING_MODE)) {
        int32_t shadingMode =
            frame_settings.find(ANDROID_SHADING_MODE).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_SHADING_MODE,
                sizeof(shadingMode), &shadingMode);
    }

    if (frame_settings.exists(ANDROID_SHADING_STRENGTH)) {
        uint8_t shadingStrength =
            frame_settings.find(ANDROID_SHADING_STRENGTH).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_SHADING_STRENGTH,
                sizeof(shadingStrength), &shadingStrength);
    }

    if (frame_settings.exists(ANDROID_STATISTICS_FACE_DETECT_MODE)) {
        uint8_t fwk_facedetectMode =
            frame_settings.find(ANDROID_STATISTICS_FACE_DETECT_MODE).data.u8[0];
        uint8_t facedetectMode =
            lookupHalName(FACEDETECT_MODES_MAP,
                sizeof(FACEDETECT_MODES_MAP), fwk_facedetectMode);
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_STATS_FACEDETECT_MODE,
                sizeof(facedetectMode), &facedetectMode);
    }

    if (frame_settings.exists(ANDROID_STATISTICS_HISTOGRAM_MODE)) {
        uint8_t histogramMode =
            frame_settings.find(ANDROID_STATISTICS_HISTOGRAM_MODE).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_STATS_HISTOGRAM_MODE,
                sizeof(histogramMode), &histogramMode);
    }

    if (frame_settings.exists(ANDROID_STATISTICS_SHARPNESS_MAP_MODE)) {
        uint8_t sharpnessMapMode =
            frame_settings.find(ANDROID_STATISTICS_SHARPNESS_MAP_MODE).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_STATS_SHARPNESS_MAP_MODE,
                sizeof(sharpnessMapMode), &sharpnessMapMode);
    }

    if (frame_settings.exists(ANDROID_TONEMAP_MODE)) {
        uint8_t tonemapMode =
            frame_settings.find(ANDROID_TONEMAP_MODE).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_TONEMAP_MODE,
                sizeof(tonemapMode), &tonemapMode);
    }
    /* Tonemap curve channels ch0 = G, ch 1 = B, ch 2 = R */
    /*All tonemap channels will have the same number of points*/
    if (frame_settings.exists(ANDROID_TONEMAP_CURVE_GREEN) &&
        frame_settings.exists(ANDROID_TONEMAP_CURVE_BLUE) &&
        frame_settings.exists(ANDROID_TONEMAP_CURVE_RED)) {
        cam_rgb_tonemap_curves tonemapCurves;
        tonemapCurves.tonemap_points_cnt = frame_settings.find(ANDROID_TONEMAP_CURVE_GREEN).count/2;

        /* ch0 = G*/
        int point = 0;
        cam_tonemap_curve_t tonemapCurveGreen;
        for (int i = 0; i < tonemapCurves.tonemap_points_cnt ; i++) {
            for (int j = 0; j < 2; j++) {
               tonemapCurveGreen.tonemap_points[i][j] =
                  frame_settings.find(ANDROID_TONEMAP_CURVE_GREEN).data.f[point];
               point++;
            }
        }
        tonemapCurves.curves[0] = tonemapCurveGreen;

        /* ch 1 = B */
        point = 0;
        cam_tonemap_curve_t tonemapCurveBlue;
        for (int i = 0; i < tonemapCurves.tonemap_points_cnt; i++) {
            for (int j = 0; j < 2; j++) {
               tonemapCurveBlue.tonemap_points[i][j] =
                  frame_settings.find(ANDROID_TONEMAP_CURVE_BLUE).data.f[point];
               point++;
            }
        }
        tonemapCurves.curves[1] = tonemapCurveBlue;

        /* ch 2 = R */
        point = 0;
        cam_tonemap_curve_t tonemapCurveRed;
        for (int i = 0; i < tonemapCurves.tonemap_points_cnt; i++) {
            for (int j = 0; j < 2; j++) {
               tonemapCurveRed.tonemap_points[i][j] =
                  frame_settings.find(ANDROID_TONEMAP_CURVE_RED).data.f[point];
               point++;
            }
        }
        tonemapCurves.curves[2] = tonemapCurveRed;

        rc = AddSetMetaEntryToBatch(hal_metadata,
                CAM_INTF_META_TONEMAP_CURVES,
                sizeof(tonemapCurves), &tonemapCurves);
    }

    if (frame_settings.exists(ANDROID_CONTROL_CAPTURE_INTENT)) {
        uint8_t captureIntent =
            frame_settings.find(ANDROID_CONTROL_CAPTURE_INTENT).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_CAPTURE_INTENT,
                sizeof(captureIntent), &captureIntent);
    }

    if (frame_settings.exists(ANDROID_BLACK_LEVEL_LOCK)) {
        uint8_t blackLevelLock =
            frame_settings.find(ANDROID_BLACK_LEVEL_LOCK).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_BLACK_LEVEL_LOCK,
                sizeof(blackLevelLock), &blackLevelLock);
    }

    if (frame_settings.exists(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE)) {
        uint8_t lensShadingMapMode =
            frame_settings.find(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_LENS_SHADING_MAP_MODE,
                sizeof(lensShadingMapMode), &lensShadingMapMode);
    }

    if (frame_settings.exists(ANDROID_CONTROL_AE_REGIONS)) {
        cam_area_t roi;
        bool reset = true;
        convertFromRegions(&roi, request->settings, ANDROID_CONTROL_AE_REGIONS);
        if (scalerCropSet) {
            reset = resetIfNeededROI(&roi, &scalerCropRegion);
        }
        if (reset) {
            rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_AEC_ROI,
                    sizeof(roi), &roi);
        }
    }

    if (frame_settings.exists(ANDROID_CONTROL_AF_REGIONS)) {
        cam_area_t roi;
        bool reset = true;
        convertFromRegions(&roi, request->settings, ANDROID_CONTROL_AF_REGIONS);
        if (scalerCropSet) {
            reset = resetIfNeededROI(&roi, &scalerCropRegion);
        }
        if (reset) {
            rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_AF_ROI,
                    sizeof(roi), &roi);
        }
    }

    if (frame_settings.exists(ANDROID_SENSOR_TEST_PATTERN_MODE)) {
        cam_test_pattern_data_t testPatternData;
        uint32_t fwk_testPatternMode = frame_settings.find(ANDROID_SENSOR_TEST_PATTERN_MODE).data.i32[0];
        uint8_t testPatternMode = lookupHalName(TEST_PATTERN_MAP,
               sizeof(TEST_PATTERN_MAP), fwk_testPatternMode);

        memset(&testPatternData, 0, sizeof(testPatternData));
        testPatternData.mode = (cam_test_pattern_mode_t)testPatternMode;
        if (testPatternMode == CAM_TEST_PATTERN_SOLID_COLOR &&
                frame_settings.exists(ANDROID_SENSOR_TEST_PATTERN_DATA)) {
            int32_t* fwk_testPatternData = frame_settings.find(
                    ANDROID_SENSOR_TEST_PATTERN_DATA).data.i32;
            testPatternData.r = fwk_testPatternData[0];
            testPatternData.b = fwk_testPatternData[3];
            switch (gCamCapability[mCameraId]->color_arrangement) {
            case CAM_FILTER_ARRANGEMENT_RGGB:
            case CAM_FILTER_ARRANGEMENT_GRBG:
                testPatternData.gr = fwk_testPatternData[1];
                testPatternData.gb = fwk_testPatternData[2];
                break;
            case CAM_FILTER_ARRANGEMENT_GBRG:
            case CAM_FILTER_ARRANGEMENT_BGGR:
                testPatternData.gr = fwk_testPatternData[2];
                testPatternData.gb = fwk_testPatternData[1];
                break;
            default:
                ALOGE("%s: color arrangement %d is not supported", __func__,
                    gCamCapability[mCameraId]->color_arrangement);
                break;
            }
        }
        rc = AddSetMetaEntryToBatch(mParameters, CAM_INTF_META_TEST_PATTERN_DATA,
            sizeof(testPatternData), &testPatternData);
    }

    if (frame_settings.exists(ANDROID_JPEG_GPS_COORDINATES)) {
        double *gps_coords =
            frame_settings.find(ANDROID_JPEG_GPS_COORDINATES).data.d;
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_JPEG_GPS_COORDINATES, sizeof(double)*3, gps_coords);
    }

    if (frame_settings.exists(ANDROID_JPEG_GPS_PROCESSING_METHOD)) {
        char gps_methods[GPS_PROCESSING_METHOD_SIZE];
        const char *gps_methods_src = (const char *)
                frame_settings.find(ANDROID_JPEG_GPS_PROCESSING_METHOD).data.u8;
        uint32_t count = frame_settings.find(
                ANDROID_JPEG_GPS_PROCESSING_METHOD).count;
        memset(gps_methods, 0, sizeof(gps_methods));
        strncpy(gps_methods, gps_methods_src, sizeof(gps_methods));
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_JPEG_GPS_PROC_METHODS, sizeof(gps_methods), gps_methods);
    }

    if (frame_settings.exists(ANDROID_JPEG_GPS_TIMESTAMP)) {
        int64_t gps_timestamp =
            frame_settings.find(ANDROID_JPEG_GPS_TIMESTAMP).data.i64[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_JPEG_GPS_TIMESTAMP, sizeof(int64_t), &gps_timestamp);
    }

    if (frame_settings.exists(ANDROID_JPEG_ORIENTATION)) {
        int32_t orientation =
            frame_settings.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_JPEG_ORIENTATION, sizeof(orientation), &orientation);
    }

    if (frame_settings.exists(ANDROID_JPEG_QUALITY)) {
        int8_t quality =
            frame_settings.find(ANDROID_JPEG_QUALITY).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_JPEG_QUALITY, sizeof(quality), &quality);
    }

    if (frame_settings.exists(ANDROID_JPEG_THUMBNAIL_QUALITY)) {
        int8_t thumb_quality =
            frame_settings.find(ANDROID_JPEG_THUMBNAIL_QUALITY).data.u8[0];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_JPEG_THUMB_QUALITY, sizeof(thumb_quality), &thumb_quality);
    }

    if (frame_settings.exists(ANDROID_JPEG_THUMBNAIL_SIZE)) {
        cam_dimension_t dim;
        dim.width = frame_settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[0];
        dim.height = frame_settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[1];
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_JPEG_THUMB_SIZE, sizeof(dim), &dim);
    }

    // Internal metadata
    if (frame_settings.exists(QCAMERA3_PRIVATEDATA_REPROCESS)) {
        uint8_t* privatedata =
            frame_settings.find(QCAMERA3_PRIVATEDATA_REPROCESS).data.u8;
        rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_META_PRIVATE_DATA,
            sizeof(uint8_t) * MAX_METADATA_PAYLOAD_SIZE, privatedata);
    }

    // EV step
    rc = AddSetMetaEntryToBatch(hal_metadata, CAM_INTF_PARM_EV_STEP,
            sizeof(cam_rational_type_t), &(gCamCapability[mCameraId]->exp_compensation_step));

    return rc;
}

/*===========================================================================
 * FUNCTION   : captureResultCb
 *
 * DESCRIPTION: Callback handler for all channels (streams, as well as metadata)
 *
 * PARAMETERS :
 *   @frame  : frame information from mm-camera-interface
 *   @buffer : actual gralloc buffer to be returned to frameworks. NULL if metadata.
 *   @userdata: userdata
 *
 * RETURN     : NONE
 *==========================================================================*/
void QCamera3HardwareInterface::captureResultCb(mm_camera_super_buf_t *metadata,
                camera3_stream_buffer_t *buffer,
                uint32_t frame_number, void *userdata)
{
    QCamera3HardwareInterface *hw = (QCamera3HardwareInterface *)userdata;
    if (hw == NULL) {
        ALOGE("%s: Invalid hw %p", __func__, hw);
        return;
    }

    hw->captureResultCb(metadata, buffer, frame_number);
    return;
}


/*===========================================================================
 * FUNCTION   : initialize
 *
 * DESCRIPTION: Pass framework callback pointers to HAL
 *
 * PARAMETERS :
 *
 *
 * RETURN     : Success : 0
 *              Failure: -ENODEV
 *==========================================================================*/

int QCamera3HardwareInterface::initialize(const struct camera3_device *device,
                                  const camera3_callback_ops_t *callback_ops)
{
    ALOGV("%s: E", __func__);
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return -ENODEV;
    }

    int rc = hw->initialize(callback_ops);
    ALOGV("%s: X", __func__);
    return rc;
}

/*===========================================================================
 * FUNCTION   : configure_streams
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     : Success: 0
 *              Failure: -EINVAL (if stream configuration is invalid)
 *                       -ENODEV (fatal error)
 *==========================================================================*/

int QCamera3HardwareInterface::configure_streams(
        const struct camera3_device *device,
        camera3_stream_configuration_t *stream_list)
{
    ALOGV("%s: E", __func__);
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return -ENODEV;
    }
    int rc = hw->configureStreams(stream_list);
    ALOGV("%s: X", __func__);
    return rc;
}

/*===========================================================================
 * FUNCTION   : register_stream_buffers
 *
 * DESCRIPTION: Register stream buffers with the device
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int QCamera3HardwareInterface::register_stream_buffers(
        const struct camera3_device *device,
        const camera3_stream_buffer_set_t *buffer_set)
{
    ALOGV("%s: E", __func__);
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return -ENODEV;
    }
    int rc = hw->registerStreamBuffers(buffer_set);
    ALOGV("%s: X", __func__);
    return rc;
}

/*===========================================================================
 * FUNCTION   : construct_default_request_settings
 *
 * DESCRIPTION: Configure a settings buffer to meet the required use case
 *
 * PARAMETERS :
 *
 *
 * RETURN     : Success: Return valid metadata
 *              Failure: Return NULL
 *==========================================================================*/
const camera_metadata_t* QCamera3HardwareInterface::
    construct_default_request_settings(const struct camera3_device *device,
                                        int type)
{

    ALOGV("%s: E", __func__);
    camera_metadata_t* fwk_metadata = NULL;
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return NULL;
    }

    fwk_metadata = hw->translateCapabilityToMetadata(type);

    ALOGV("%s: X", __func__);
    return fwk_metadata;
}

/*===========================================================================
 * FUNCTION   : process_capture_request
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/
int QCamera3HardwareInterface::process_capture_request(
                    const struct camera3_device *device,
                    camera3_capture_request_t *request)
{
    ALOGV("%s: E", __func__);
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return -EINVAL;
    }

    int rc = hw->processCaptureRequest(request);
    ALOGV("%s: X", __func__);
    return rc;
}

/*===========================================================================
 * FUNCTION   : dump
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/

void QCamera3HardwareInterface::dump(
                const struct camera3_device *device, int fd)
{
    ALOGV("%s: E", __func__);
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return;
    }

    hw->dump(fd);
    ALOGV("%s: X", __func__);
    return;
}

/*===========================================================================
 * FUNCTION   : flush
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/

int QCamera3HardwareInterface::flush(
                const struct camera3_device *device)
{
    int rc;
    ALOGV("%s: E", __func__);
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return -EINVAL;
    }

    rc = hw->flush();
    ALOGV("%s: X", __func__);
    return rc;
}

/*===========================================================================
 * FUNCTION   : close_camera_device
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/
int QCamera3HardwareInterface::close_camera_device(struct hw_device_t* device)
{
    ALOGV("%s: E", __func__);
    int ret = NO_ERROR;
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(
            reinterpret_cast<camera3_device_t *>(device)->priv);
    if (!hw) {
        ALOGE("NULL camera device");
        return BAD_VALUE;
    }
    delete hw;

    pthread_mutex_lock(&mCameraSessionLock);
    mCameraSessionActive = 0;
    pthread_mutex_unlock(&mCameraSessionLock);
    ALOGV("%s: X", __func__);
    return ret;
}

/*===========================================================================
 * FUNCTION   : getWaveletDenoiseProcessPlate
 *
 * DESCRIPTION: query wavelet denoise process plate
 *
 * PARAMETERS : None
 *
 * RETURN     : WNR prcocess plate vlaue
 *==========================================================================*/
cam_denoise_process_type_t QCamera3HardwareInterface::getWaveletDenoiseProcessPlate()
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.denoise.process.plates", prop, "0");
    int processPlate = atoi(prop);
    switch(processPlate) {
    case 0:
        return CAM_WAVELET_DENOISE_YCBCR_PLANE;
    case 1:
        return CAM_WAVELET_DENOISE_CBCR_ONLY;
    case 2:
        return CAM_WAVELET_DENOISE_STREAMLINE_YCBCR;
    case 3:
        return CAM_WAVELET_DENOISE_STREAMLINED_CBCR;
    default:
        return CAM_WAVELET_DENOISE_STREAMLINE_YCBCR;
    }
}

/*===========================================================================
 * FUNCTION   : needRotationReprocess
 *
 * DESCRIPTION: if rotation needs to be done by reprocess in pp
 *
 * PARAMETERS : none
 *
 * RETURN     : true: needed
 *              false: no need
 *==========================================================================*/
bool QCamera3HardwareInterface::needRotationReprocess()
{
    if ((gCamCapability[mCameraId]->qcom_supported_feature_mask & CAM_QCOM_FEATURE_ROTATION) > 0) {
        // current rotation is not zero, and pp has the capability to process rotation
        ALOGD("%s: need do reprocess for rotation", __func__);
        return true;
    }

    return false;
}

/*===========================================================================
 * FUNCTION   : needReprocess
 *
 * DESCRIPTION: if reprocess in needed
 *
 * PARAMETERS : none
 *
 * RETURN     : true: needed
 *              false: no need
 *==========================================================================*/
bool QCamera3HardwareInterface::needReprocess()
{
    if (gCamCapability[mCameraId]->min_required_pp_mask > 0) {
        // TODO: add for ZSL HDR later
        // pp module has min requirement for zsl reprocess, or WNR in ZSL mode
        ALOGD("%s: need do reprocess for ZSL WNR or min PP reprocess", __func__);
        return true;
    }
    return needRotationReprocess();
}

/*===========================================================================
 * FUNCTION   : addOfflineReprocChannel
 *
 * DESCRIPTION: add a reprocess channel that will do reprocess on frames
 *              coming from input channel
 *
 * PARAMETERS :
 *   @pInputChannel : ptr to input channel whose frames will be post-processed
 *
 * RETURN     : Ptr to the newly created channel obj. NULL if failed.
 *==========================================================================*/
QCamera3ReprocessChannel *QCamera3HardwareInterface::addOfflineReprocChannel(
              QCamera3Channel *pInputChannel, QCamera3PicChannel *picChHandle, metadata_buffer_t *metadata)
{
    int32_t rc = NO_ERROR;
    QCamera3ReprocessChannel *pChannel = NULL;
    if (pInputChannel == NULL) {
        ALOGE("%s: input channel obj is NULL", __func__);
        return NULL;
    }

    pChannel = new QCamera3ReprocessChannel(mCameraHandle->camera_handle,
            mCameraHandle->ops, NULL, pInputChannel->mPaddingInfo, this, picChHandle);
    if (NULL == pChannel) {
        ALOGE("%s: no mem for reprocess channel", __func__);
        return NULL;
    }

    rc = pChannel->initialize();
    if (rc != NO_ERROR) {
        ALOGE("%s: init reprocess channel failed, ret = %d", __func__, rc);
        delete pChannel;
        return NULL;
    }

    // pp feature config
    cam_pp_feature_config_t pp_config;
    memset(&pp_config, 0, sizeof(cam_pp_feature_config_t));

    if (IS_PARM_VALID(CAM_INTF_META_EDGE_MODE, metadata)) {
        cam_edge_application_t *edge = (cam_edge_application_t *)
                POINTER_OF(CAM_INTF_META_EDGE_MODE, metadata);
        if (edge->edge_mode != CAM_EDGE_MODE_OFF) {
            pp_config.feature_mask |= CAM_QCOM_FEATURE_SHARPNESS;
            pp_config.sharpness = edge->sharpness;
        }
    }

    if (IS_PARM_VALID(CAM_INTF_META_NOISE_REDUCTION_MODE, metadata)) {
        uint8_t *noise_mode = (uint8_t *)POINTER_OF(
                CAM_INTF_META_NOISE_REDUCTION_MODE, metadata);
        if (*noise_mode != CAM_NOISE_REDUCTION_MODE_OFF) {
            pp_config.feature_mask |= CAM_QCOM_FEATURE_DENOISE2D;
            pp_config.denoise2d.denoise_enable = 1;
            pp_config.denoise2d.process_plates = getWaveletDenoiseProcessPlate();
        }
    }

    if (IS_PARM_VALID(CAM_INTF_META_JPEG_ORIENTATION, metadata)) {
        int32_t *rotation = (int32_t *)POINTER_OF(
                CAM_INTF_META_JPEG_ORIENTATION, metadata);

        if (needRotationReprocess()) {
            pp_config.feature_mask |= CAM_QCOM_FEATURE_ROTATION;
            if (*rotation == 0) {
                pp_config.rotation = ROTATE_0;
            } else if (*rotation == 90) {
                pp_config.rotation = ROTATE_90;
            } else if (*rotation == 180) {
                pp_config.rotation = ROTATE_180;
            } else if (*rotation == 270) {
                pp_config.rotation = ROTATE_270;
            }
        }
    }

    rc = pChannel->addReprocStreamsFromSource(pp_config,
                                             pInputChannel,
                                             mMetadataChannel);

    if (rc != NO_ERROR) {
        delete pChannel;
        return NULL;
    }
    return pChannel;
}

}; //end namespace qcamera
