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

#define LOG_TAG "QCamera3Channel"
//#define LOG_NDEBUG 0
#include <fcntl.h>
#include <stdlib.h>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <hardware/camera3.h>
#include <system/camera_metadata.h>
#include <gralloc_priv.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <cutils/properties.h>
#include "QCamera3Channel.h"

using namespace android;

#define MIN_STREAMING_BUFFER_NUM 7+11

namespace qcamera {
static const char ExifAsciiPrefix[] =
    { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };          // "ASCII\0\0\0"
static const char ExifUndefinedPrefix[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // "\0\0\0\0\0\0\0\0"

#define EXIF_ASCII_PREFIX_SIZE           8   //(sizeof(ExifAsciiPrefix))
#define FOCAL_LENGTH_DECIMAL_PRECISION   1000

/*===========================================================================
 * FUNCTION   : QCamera3Channel
 *
 * DESCRIPTION: constrcutor of QCamera3Channel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3Channel::QCamera3Channel(uint32_t cam_handle,
                               mm_camera_ops_t *cam_ops,
                               channel_cb_routine cb_routine,
                               cam_padding_info_t *paddingInfo,
                               void *userData)
{
    m_camHandle = cam_handle;
    m_camOps = cam_ops;
    m_bIsActive = false;

    m_handle = 0;
    m_numStreams = 0;
    memset(mStreams, 0, sizeof(mStreams));
    mUserData = userData;

    mStreamInfoBuf = NULL;
    mChannelCB = cb_routine;
    mPaddingInfo = paddingInfo;
}

/*===========================================================================
 * FUNCTION   : QCamera3Channel
 *
 * DESCRIPTION: default constrcutor of QCamera3Channel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3Channel::QCamera3Channel()
{
    m_camHandle = 0;
    m_camOps = NULL;
    m_bIsActive = false;

    m_handle = 0;
    m_numStreams = 0;
    memset(mStreams, 0, sizeof(mStreams));
    mUserData = NULL;

    mStreamInfoBuf = NULL;
    mChannelCB = NULL;
    mPaddingInfo = NULL;
}

/*===========================================================================
 * FUNCTION   : ~QCamera3Channel
 *
 * DESCRIPTION: destructor of QCamera3Channel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3Channel::~QCamera3Channel()
{
    if (m_bIsActive)
        stop();

    for (int i = 0; i < m_numStreams; i++) {
        if (mStreams[i] != NULL) {
            delete mStreams[i];
            mStreams[i] = 0;
        }
    }
    if (m_handle) {
        m_camOps->delete_channel(m_camHandle, m_handle);
        ALOGE("%s: deleting channel %d", __func__, m_handle);
        m_handle = 0;
    }
    m_numStreams = 0;
}

/*===========================================================================
 * FUNCTION   : init
 *
 * DESCRIPTION: initialization of channel
 *
 * PARAMETERS :
 *   @attr    : channel bundle attribute setting
 *   @dataCB  : data notify callback
 *   @userData: user data ptr
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3Channel::init(mm_camera_channel_attr_t *attr,
                             mm_camera_buf_notify_t dataCB)
{
    m_handle = m_camOps->add_channel(m_camHandle,
                                      attr,
                                      dataCB,
                                      this);
    if (m_handle == 0) {
        ALOGE("%s: Add channel failed", __func__);
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : addStream
 *
 * DESCRIPTION: add a stream into channel
 *
 * PARAMETERS :
 *   @allocator      : stream related buffer allocator
 *   @streamInfoBuf  : ptr to buf that constains stream info
 *   @minStreamBufNum: number of stream buffers needed
 *   @paddingInfo    : padding information
 *   @stream_cb      : stream data notify callback
 *   @userdata       : user data ptr
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3Channel::addStream(cam_stream_type_t streamType,
                                  cam_format_t streamFormat,
                                  cam_dimension_t streamDim,
                                  uint8_t minStreamBufNum)
{
    int32_t rc = NO_ERROR;

    if (m_numStreams >= 1) {
        ALOGE("%s: Only one stream per channel supported in v3 Hal", __func__);
        return BAD_VALUE;
    }

    if (m_numStreams >= MAX_STREAM_NUM_IN_BUNDLE) {
        ALOGE("%s: stream number (%d) exceeds max limit (%d)",
              __func__, m_numStreams, MAX_STREAM_NUM_IN_BUNDLE);
        return BAD_VALUE;
    }
    QCamera3Stream *pStream = new QCamera3Stream(m_camHandle,
                                               m_handle,
                                               m_camOps,
                                               mPaddingInfo,
                                               this);
    if (pStream == NULL) {
        ALOGE("%s: No mem for Stream", __func__);
        return NO_MEMORY;
    }

    rc = pStream->init(streamType, streamFormat, streamDim, NULL, minStreamBufNum,
                                                    streamCbRoutine, this);
    if (rc == 0) {
        mStreams[m_numStreams] = pStream;
        m_numStreams++;
    } else {
        delete pStream;
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : start
 *
 * DESCRIPTION: start channel, which will start all streams belong to this channel
 *
 * PARAMETERS :
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3Channel::start()
{
    int32_t rc = NO_ERROR;

    if (m_numStreams > 1) {
        ALOGE("%s: bundle not supported", __func__);
    } else if (m_numStreams == 0) {
        return NO_INIT;
    }

    if(m_bIsActive) {
        ALOGD("%s: Attempt to start active channel", __func__);
        return rc;
    }

    for (int i = 0; i < m_numStreams; i++) {
        if (mStreams[i] != NULL) {
            mStreams[i]->start();
        }
    }
    rc = m_camOps->start_channel(m_camHandle, m_handle);

    if (rc != NO_ERROR) {
        for (int i = 0; i < m_numStreams; i++) {
            if (mStreams[i] != NULL) {
                mStreams[i]->stop();
            }
        }
    } else {
        m_bIsActive = true;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : stop
 *
 * DESCRIPTION: stop a channel, which will stop all streams belong to this channel
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3Channel::stop()
{
    int32_t rc = NO_ERROR;
    if(!m_bIsActive) {
        ALOGE("%s: Attempt to stop inactive channel",__func__);
        return rc;
    }

    for (int i = 0; i < m_numStreams; i++) {
        if (mStreams[i] != NULL) {
            mStreams[i]->stop();
        }
    }

    rc = m_camOps->stop_channel(m_camHandle, m_handle);

    m_bIsActive = false;
    return rc;
}

/*===========================================================================
 * FUNCTION   : bufDone
 *
 * DESCRIPTION: return a stream buf back to kernel
 *
 * PARAMETERS :
 *   @recvd_frame  : stream buf frame to be returned
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3Channel::bufDone(mm_camera_super_buf_t *recvd_frame)
{
    int32_t rc = NO_ERROR;
    for (int i = 0; i < recvd_frame->num_bufs; i++) {
         if (recvd_frame->bufs[i] != NULL) {
             for (int j = 0; j < m_numStreams; j++) {
                 if (mStreams[j] != NULL &&
                     mStreams[j]->getMyHandle() == recvd_frame->bufs[i]->stream_id) {
                     rc = mStreams[j]->bufDone(recvd_frame->bufs[i]->buf_idx);
                     break; // break loop j
                 }
             }
         }
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : getStreamTypeMask
 *
 * DESCRIPTION: Get bit mask of all stream types in this channel
 *
 * PARAMETERS : None
 *
 * RETURN     : Bit mask of all stream types in this channel
 *==========================================================================*/
uint32_t QCamera3Channel::getStreamTypeMask()
{
    uint32_t mask = 0;
    for (int i = 0; i < m_numStreams; i++) {
       mask |= (0x1 << mStreams[i]->getMyType());
    }
    return mask;
}

/*===========================================================================
 * FUNCTION   : getStreamID
 *
 * DESCRIPTION: Get StreamID of requested stream type
 *
 * PARAMETERS : streamMask
 *
 * RETURN     : Stream ID
 *==========================================================================*/
uint32_t QCamera3Channel::getStreamID(uint32_t streamMask)
{
    uint32_t streamID = 0;
    for (int i = 0; i < m_numStreams; i++) {
        if (streamMask == (uint32_t )(0x1 << mStreams[i]->getMyType())) {
            streamID = mStreams[i]->getMyServerID();
            break;
        }
    }
    return streamID;
}

/*===========================================================================
 * FUNCTION   : getStreamByHandle
 *
 * DESCRIPTION: return stream object by stream handle
 *
 * PARAMETERS :
 *   @streamHandle : stream handle
 *
 * RETURN     : stream object. NULL if not found
 *==========================================================================*/
QCamera3Stream *QCamera3Channel::getStreamByHandle(uint32_t streamHandle)
{
    for (int i = 0; i < m_numStreams; i++) {
        if (mStreams[i] != NULL && mStreams[i]->getMyHandle() == streamHandle) {
            return mStreams[i];
        }
    }
    return NULL;
}

/*===========================================================================
 * FUNCTION   : getStreamByIndex
 *
 * DESCRIPTION: return stream object by index
 *
 * PARAMETERS :
 *   @streamHandle : stream handle
 *
 * RETURN     : stream object. NULL if not found
 *==========================================================================*/
QCamera3Stream *QCamera3Channel::getStreamByIndex(uint8_t index)
{
    if (index < m_numStreams) {
        return mStreams[index];
    }
    return NULL;
}

/*===========================================================================
 * FUNCTION   : streamCbRoutine
 *
 * DESCRIPTION: callback routine for stream
 *
 * PARAMETERS :
 *   @streamHandle : stream handle
 *
 * RETURN     : stream object. NULL if not found
 *==========================================================================*/
void QCamera3Channel::streamCbRoutine(mm_camera_super_buf_t *super_frame,
                QCamera3Stream *stream, void *userdata)
{
    QCamera3Channel *channel = (QCamera3Channel *)userdata;
    if (channel == NULL) {
        ALOGE("%s: invalid channel pointer", __func__);
        return;
    }
    channel->streamCbRoutine(super_frame, stream);
}

/*===========================================================================
 * FUNCTION   : QCamera3RegularChannel
 *
 * DESCRIPTION: constrcutor of QCamera3RegularChannel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *   @cb_routine : callback routine to frame aggregator
 *   @stream     : camera3_stream_t structure
 *   @stream_type: Channel stream type
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3RegularChannel::QCamera3RegularChannel(uint32_t cam_handle,
                    mm_camera_ops_t *cam_ops,
                    channel_cb_routine cb_routine,
                    cam_padding_info_t *paddingInfo,
                    void *userData,
                    camera3_stream_t *stream,
                    cam_stream_type_t stream_type) :
                        QCamera3Channel(cam_handle, cam_ops, cb_routine,
                                                paddingInfo, userData),
                        mCamera3Stream(stream),
                        mNumBufs(0),
                        mStreamType(stream_type)
{
}

/*===========================================================================
 * FUNCTION   : ~QCamera3RegularChannel
 *
 * DESCRIPTION: destructor of QCamera3RegularChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3RegularChannel::~QCamera3RegularChannel()
{
    mMemory.unregisterBuffers();
}

/*===========================================================================
 * FUNCTION   : initialize
 *
 * DESCRIPTION: Initialize and add camera channel & stream
 *
 * PARAMETERS :
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/

int32_t QCamera3RegularChannel::initialize()
{
    int32_t rc = NO_ERROR;
    cam_format_t streamFormat;
    cam_dimension_t streamDim;

    if (NULL == mCamera3Stream) {
        ALOGE("%s: Camera stream uninitialized", __func__);
        return NO_INIT;
    }

    if (1 <= m_numStreams) {
        // Only one stream per channel supported in v3 Hal
        return NO_ERROR;
    }

    rc = init(NULL, NULL);
    if (rc < 0) {
        ALOGE("%s: init failed", __func__);
        return rc;
    }

    mNumBufs = CAM_MAX_NUM_BUFS_PER_STREAM;

    if (mCamera3Stream->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
        if (mStreamType ==  CAM_STREAM_TYPE_VIDEO) {
            streamFormat = CAM_FORMAT_YUV_420_NV12;
        } else if (mStreamType == CAM_STREAM_TYPE_PREVIEW) {
            streamFormat = CAM_FORMAT_YUV_420_NV21;
        } else {
            //TODO: Add a new flag in libgralloc for ZSL buffers, and its size needs
            // to be properly aligned and padded.
            streamFormat = CAM_FORMAT_YUV_420_NV21;
        }
    } else if(mCamera3Stream->format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
         streamFormat = CAM_FORMAT_YUV_420_NV21;
    } else if (mCamera3Stream->format == HAL_PIXEL_FORMAT_RAW_OPAQUE ||
            mCamera3Stream->format == HAL_PIXEL_FORMAT_RAW16) {
        // Bayer pattern doesn't matter here.
        // All CAMIF raw format uses 10bit.
        streamFormat = CAM_FORMAT_BAYER_QCOM_RAW_10BPP_GBRG;
    } else {

        //TODO: Fail for other types of streams for now
        ALOGE("%s: format is not IMPLEMENTATION_DEFINED or flexible", __func__);
        return -EINVAL;
    }

    streamDim.width = mCamera3Stream->width;
    streamDim.height = mCamera3Stream->height;

    rc = QCamera3Channel::addStream(mStreamType,
            streamFormat,
            streamDim,
            mNumBufs);

    return rc;
}

/*===========================================================================
 * FUNCTION   : start
 *
 * DESCRIPTION: start a regular channel
 *
 * PARAMETERS :
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3RegularChannel::start()
{
    int32_t rc = NO_ERROR;

    if (0 < mMemory.getCnt()) {
        rc = QCamera3Channel::start();
    }

    return rc;
}
/*===========================================================================
 * FUNCTION   : getInternalFormatBuffer
 *
 * DESCRIPTION: return buffer in the internal format structure
 *
 * PARAMETERS :
 *   @streamHandle : buffer handle
 *
 * RETURN     : stream object. NULL if not found
 *==========================================================================*/
mm_camera_buf_def_t* QCamera3RegularChannel::getInternalFormatBuffer(
                                            buffer_handle_t * buffer)
{
    int32_t index;
    if(buffer == NULL)
        return NULL;
    index = mMemory.getMatchBufIndex((void*)buffer);
    if(index < 0) {
        ALOGE("%s: Could not find object among registered buffers",__func__);
        return NULL;
    }
    return mStreams[0]->getInternalFormatBuffer(index);
}

/*===========================================================================
 * FUNCTION   : request
 *
 * DESCRIPTION: process a request from camera service. Stream on if ncessary.
 *
 * PARAMETERS :
 *   @buffer  : buffer to be filled for this request
 *
 * RETURN     : 0 on a success start of capture
 *              -EINVAL on invalid input
 *              -ENODEV on serious error
 *==========================================================================*/
int32_t QCamera3RegularChannel::request(buffer_handle_t *buffer, uint32_t frameNumber)
{
    //FIX ME: Return buffer back in case of failures below.

    int32_t rc = NO_ERROR;
    int index;

    if (NULL == buffer) {
        ALOGE("%s: Invalid buffer in channel request", __func__);
        return BAD_VALUE;
    }

    if(!m_bIsActive) {
        rc = registerBuffer(buffer);
        if (NO_ERROR != rc) {
            ALOGE("%s: On-the-fly buffer registration failed %d",
                    __func__, rc);
            return rc;
        }

        rc = start();
        if (NO_ERROR != rc) {
            return rc;
        }
    } else {
        ALOGV("%s: Request on an existing stream",__func__);
    }

    index = mMemory.getMatchBufIndex((void*)buffer);
    if(index < 0) {
        rc = registerBuffer(buffer);
        if (NO_ERROR != rc) {
            ALOGE("%s: On-the-fly buffer registration failed %d",
                    __func__, rc);
            return rc;
        }

        index = mMemory.getMatchBufIndex((void*)buffer);
        if (index < 0) {
            ALOGE("%s: Could not find object among registered buffers",
                    __func__);
            return DEAD_OBJECT;
        }
    }

    rc = mStreams[0]->bufDone(index);
    if(rc != NO_ERROR) {
        ALOGE("%s: Failed to Q new buffer to stream",__func__);
        return rc;
    }

    rc = mMemory.markFrameNumber(index, frameNumber);
    return rc;
}

/*===========================================================================
 * FUNCTION   : registerBuffer
 *
 * DESCRIPTION: register streaming buffer to the channel object
 *
 * PARAMETERS :
 *   @buffer     : buffer to be registered
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3RegularChannel::registerBuffer(buffer_handle_t *buffer)
{
    int rc = 0;

    if ((uint32_t)mMemory.getCnt() > (mNumBufs - 1)) {
        ALOGE("%s: Trying to register more buffers than initially requested",
                __func__);
        return BAD_VALUE;
    }

    if (0 == m_numStreams) {
        rc = initialize();
        if (rc != NO_ERROR) {
            ALOGE("%s: Couldn't initialize camera stream %d",
                    __func__, rc);
            return rc;
        }
    }

    rc = mMemory.registerBuffer(buffer);
    if (ALREADY_EXISTS == rc) {
        return NO_ERROR;
    } else if (NO_ERROR != rc) {
        ALOGE("%s: Buffer %p couldn't be registered %d", __func__, buffer, rc);
        return rc;
    }

    return rc;
}

void QCamera3RegularChannel::streamCbRoutine(
                            mm_camera_super_buf_t *super_frame,
                            QCamera3Stream *stream)
{
    //FIXME Q Buf back in case of error?
    uint8_t frameIndex;
    buffer_handle_t *resultBuffer;
    int32_t resultFrameNumber;
    camera3_stream_buffer_t result;

    if(!super_frame) {
         ALOGE("%s: Invalid Super buffer",__func__);
         return;
    }

    if(super_frame->num_bufs != 1) {
         ALOGE("%s: Multiple streams are not supported",__func__);
         return;
    }
    if(super_frame->bufs[0] == NULL ) {
         ALOGE("%s: Error, Super buffer frame does not contain valid buffer",
                  __func__);
         return;
    }

    frameIndex = (uint8_t)super_frame->bufs[0]->buf_idx;
    if(frameIndex >= mNumBufs) {
         ALOGE("%s: Error, Invalid index for buffer",__func__);
         if(stream) {
             stream->bufDone(frameIndex);
         }
         return;
    }

    ////Use below data to issue framework callback
    resultBuffer = (buffer_handle_t *)mMemory.getBufferHandle(frameIndex);
    resultFrameNumber = mMemory.getFrameNumber(frameIndex);

    result.stream = mCamera3Stream;
    result.buffer = resultBuffer;
    result.status = CAMERA3_BUFFER_STATUS_OK;
    result.acquire_fence = -1;
    result.release_fence = -1;

    mChannelCB(NULL, &result, resultFrameNumber, mUserData);
    free(super_frame);
    return;
}

QCamera3Memory* QCamera3RegularChannel::getStreamBufs(uint32_t /*len*/)
{
    return &mMemory;
}

int QCamera3RegularChannel::kMaxBuffers = 7;

QCamera3MetadataChannel::QCamera3MetadataChannel(uint32_t cam_handle,
                    mm_camera_ops_t *cam_ops,
                    channel_cb_routine cb_routine,
                    cam_padding_info_t *paddingInfo,
                    void *userData) :
                        QCamera3Channel(cam_handle, cam_ops,
                                cb_routine, paddingInfo, userData),
                        mMemory(NULL)
{
}

QCamera3MetadataChannel::~QCamera3MetadataChannel()
{
    if (m_bIsActive)
        stop();

    if (mMemory) {
        mMemory->deallocate();
        delete mMemory;
        mMemory = NULL;
    }
}

int32_t QCamera3MetadataChannel::initialize()
{
    int32_t rc;
    cam_dimension_t streamDim;

    if (mMemory || m_numStreams > 0) {
        ALOGE("%s: metadata channel already initialized", __func__);
        return -EINVAL;
    }

    rc = init(NULL, NULL);
    if (rc < 0) {
        ALOGE("%s: init failed", __func__);
        return rc;
    }

    streamDim.width = sizeof(metadata_buffer_t),
    streamDim.height = 1;
    rc = QCamera3Channel::addStream(CAM_STREAM_TYPE_METADATA, CAM_FORMAT_MAX,
        streamDim, MIN_STREAMING_BUFFER_NUM);
    if (rc < 0) {
        ALOGE("%s: addStream failed", __func__);
    }
    return rc;
}

int32_t QCamera3MetadataChannel::request(buffer_handle_t * /*buffer*/,
                                                uint32_t /*frameNumber*/)
{
    if (!m_bIsActive) {
        return start();
    }
    else
        return 0;
}

void QCamera3MetadataChannel::streamCbRoutine(
                        mm_camera_super_buf_t *super_frame,
                        QCamera3Stream * /*stream*/)
{
    uint32_t requestNumber = 0;
    if (super_frame == NULL || super_frame->num_bufs != 1) {
        ALOGE("%s: super_frame is not valid", __func__);
        return;
    }
    mChannelCB(super_frame, NULL, requestNumber, mUserData);
}

QCamera3Memory* QCamera3MetadataChannel::getStreamBufs(uint32_t len)
{
    int rc;
    if (len < sizeof(metadata_buffer_t)) {
        ALOGE("%s: size doesn't match %d vs %d", __func__,
                len, sizeof(metadata_buffer_t));
        return NULL;
    }
    mMemory = new QCamera3HeapMemory();
    if (!mMemory) {
        ALOGE("%s: unable to create metadata memory", __func__);
        return NULL;
    }
    rc = mMemory->allocate(MIN_STREAMING_BUFFER_NUM, len, true);
    if (rc < 0) {
        ALOGE("%s: unable to allocate metadata memory", __func__);
        delete mMemory;
        mMemory = NULL;
        return NULL;
    }
    memset(mMemory->getPtr(0), 0, sizeof(metadata_buffer_t));
    return mMemory;
}

void QCamera3MetadataChannel::putStreamBufs()
{
    mMemory->deallocate();
    delete mMemory;
    mMemory = NULL;
}
/*************************************************************************************/
// RAW Channel related functions
int QCamera3RawChannel::kMaxBuffers = 7;

QCamera3RawChannel::QCamera3RawChannel(uint32_t cam_handle,
                    mm_camera_ops_t *cam_ops,
                    channel_cb_routine cb_routine,
                    cam_padding_info_t *paddingInfo,
                    void *userData,
                    camera3_stream_t *stream,
                    bool raw_16) :
                        QCamera3RegularChannel(cam_handle, cam_ops,
                                cb_routine, paddingInfo, userData, stream,
                                CAM_STREAM_TYPE_RAW),
                        mIsRaw16(raw_16)
{
    char prop[PROPERTY_VALUE_MAX];
    property_get("persist.camera.raw.dump", prop, "0");
    mRawDump = atoi(prop);
}

QCamera3RawChannel::~QCamera3RawChannel()
{
}

void QCamera3RawChannel::streamCbRoutine(
                        mm_camera_super_buf_t *super_frame,
                        QCamera3Stream * stream)
{
    /* Move this back down once verified */
    if (mRawDump)
        dumpRawSnapshot(super_frame->bufs[0]);

    if (mIsRaw16)
        convertToRaw16(super_frame->bufs[0]);

    //Make sure cache coherence because extra processing is done
    mMemory.cleanInvalidateCache(super_frame->bufs[0]->buf_idx);

    QCamera3RegularChannel::streamCbRoutine(super_frame, stream);
    return;
}

void QCamera3RawChannel::dumpRawSnapshot(mm_camera_buf_def_t *frame)
{
   QCamera3Stream *stream = getStreamByIndex(0);
   char buf[32];
   memset(buf, 0, sizeof(buf));
   cam_dimension_t dim;
   memset(&dim, 0, sizeof(dim));
   stream->getFrameDimension(dim);

   cam_frame_len_offset_t offset;
   memset(&offset, 0, sizeof(cam_frame_len_offset_t));
   stream->getFrameOffset(offset);
   snprintf(buf, sizeof(buf), "/data/r_%d_%dx%d.raw",
            frame->frame_idx, dim.width, dim.height);

   int file_fd = open(buf, O_RDWR| O_CREAT, 0777);
   if (file_fd >= 0) {
      int written_len = write(file_fd, frame->buffer, offset.frame_len);
      ALOGE("%s: written number of bytes %d", __func__, written_len);
      close(file_fd);
   } else {
      ALOGE("%s: failed to open file to dump image", __func__);
   }

}

void QCamera3RawChannel::convertToRaw16(mm_camera_buf_def_t *frame)
{
    // Convert image buffer from Opaque raw format to RAW16 format
    // 10bit Opaque raw is stored in the format of:
    // 0000 - p5 - p4 - p3 - p2 - p1 - p0
    // where p0 to p5 are 6 pixels (each is 10bit)_and most significant
    // 4 bits are 0s. Each 64bit word contains 6 pixels.

    QCamera3Stream *stream = getStreamByIndex(0);
    cam_dimension_t dim;
    memset(&dim, 0, sizeof(dim));
    stream->getFrameDimension(dim);

    cam_frame_len_offset_t offset;
    memset(&offset, 0, sizeof(cam_frame_len_offset_t));
    stream->getFrameOffset(offset);

    uint32_t raw16_stride = (dim.width + 15) & ~15;
    uint16_t* raw16_buffer = (uint16_t *)frame->buffer;

    // In-place format conversion.
    // Raw16 format always occupy more memory than opaque raw10.
    // Convert to Raw16 by iterating through all pixels from bottom-right
    // to top-left of the image.
    // One special notes:
    // 1. Cross-platform raw16's stride is 16 pixels.
    // 2. Opaque raw10's stride is 6 pixels, and aligned to 16 bytes.
    for (int y = dim.height-1; y >= 0; y--) {
        uint64_t* row_start = (uint64_t *)frame->buffer +
            y * offset.mp[0].stride / 8;
        for (int x = dim.width-1;  x >= 0; x--) {
            uint16_t raw16_pixel = 0x3FF & (row_start[x/6] >> (10*(x%6)));
            raw16_buffer[y*raw16_stride+x] = raw16_pixel;
        }
    }
}

/*************************************************************************************/

/*===========================================================================
 * FUNCTION   : jpegEvtHandle
 *
 * DESCRIPTION: Function registerd to mm-jpeg-interface to handle jpeg events.
                Construct result payload and call mChannelCb to deliver buffer
                to framework.
 *
 * PARAMETERS :
 *   @status    : status of jpeg job
 *   @client_hdl: jpeg client handle
 *   @jobId     : jpeg job Id
 *   @p_ouput   : ptr to jpeg output result struct
 *   @userdata  : user data ptr
 *
 * RETURN     : none
 *==========================================================================*/
void QCamera3PicChannel::jpegEvtHandle(jpeg_job_status_t status,
                                              uint32_t /*client_hdl*/,
                                              uint32_t jobId,
                                              mm_jpeg_output_t *p_output,
                                              void *userdata)
{
    buffer_handle_t *resultBuffer, *jpegBufferHandle;
    int32_t resultFrameNumber;
    int resultStatus = CAMERA3_BUFFER_STATUS_OK;
    camera3_stream_buffer_t result;
    camera3_jpeg_blob_t jpegHeader;
    char* jpeg_eof = 0;
    int maxJpegSize;
    QCamera3PicChannel *obj = (QCamera3PicChannel *)userdata;
    if (obj) {

        //Release any cached metabuffer information
        if (obj->mMetaFrame != NULL && obj->m_pMetaChannel != NULL) {
            ((QCamera3MetadataChannel*)(obj->m_pMetaChannel))->bufDone(obj->mMetaFrame);
            obj->mMetaFrame = NULL;
            obj->m_pMetaChannel = NULL;
        } else {
            ALOGE("%s: Meta frame was NULL", __func__);
        }
        //Construct payload for process_capture_result. Call mChannelCb

        qcamera_jpeg_data_t *job = obj->m_postprocessor.findJpegJobByJobId(jobId);

        if ((job == NULL) || (status == JPEG_JOB_STATUS_ERROR)) {
            ALOGE("%s: Error in jobId: (%d) with status: %d", __func__, jobId, status);
            resultStatus = CAMERA3_BUFFER_STATUS_ERROR;
        }

        //Construct jpeg transient header of type camera3_jpeg_blob_t
        //Append at the end of jpeg image of buf_filled_len size

        jpegHeader.jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
        jpegHeader.jpeg_size = p_output->buf_filled_len;


        char* jpeg_buf = (char *)p_output->buf_vaddr;

        // Gralloc buffer may have additional padding for 4K page size
        // Follow size guidelines based on spec since framework relies
        // on that to reach end of buffer and with it the header

        //Handle same as resultBuffer, but for readablity
        jpegBufferHandle =
            (buffer_handle_t *)obj->mMemory.getBufferHandle(obj->mCurrentBufIndex);

        maxJpegSize = ((private_handle_t*)(*jpegBufferHandle))->width;
        if (maxJpegSize > obj->mMemory.getSize(obj->mCurrentBufIndex)) {
            maxJpegSize = obj->mMemory.getSize(obj->mCurrentBufIndex);
        }

        jpeg_eof = &jpeg_buf[maxJpegSize-sizeof(jpegHeader)];
        memcpy(jpeg_eof, &jpegHeader, sizeof(jpegHeader));
        obj->mMemory.cleanInvalidateCache(obj->mCurrentBufIndex);

        ////Use below data to issue framework callback
        resultBuffer = (buffer_handle_t *)obj->mMemory.getBufferHandle(obj->mCurrentBufIndex);
        resultFrameNumber = obj->mMemory.getFrameNumber(obj->mCurrentBufIndex);

        result.stream = obj->mCamera3Stream;
        result.buffer = resultBuffer;
        result.status = resultStatus;
        result.acquire_fence = -1;
        result.release_fence = -1;

        ALOGV("%s: Issue Callback", __func__);
        obj->mChannelCB(NULL, &result, resultFrameNumber, obj->mUserData);

        // release internal data for jpeg job
        if (job != NULL) {
            obj->m_postprocessor.releaseJpegJobData(job);
            free(job);
        }
        return;
        // }
    } else {
        ALOGE("%s: Null userdata in jpeg callback", __func__);
    }
}

QCamera3PicChannel::QCamera3PicChannel(uint32_t cam_handle,
                    mm_camera_ops_t *cam_ops,
                    channel_cb_routine cb_routine,
                    cam_padding_info_t *paddingInfo,
                    void *userData,
                    camera3_stream_t *stream) :
                        QCamera3Channel(cam_handle, cam_ops, cb_routine,
                        paddingInfo, userData),
                        m_postprocessor(this),
                        mCamera3Stream(stream),
                        mNumBufs(0),
                        mCurrentBufIndex(-1),
                        mYuvMemory(NULL),
                        mMetaFrame(NULL)
{
    mYuvWidth = stream->width;
    mYuvHeight = stream->height;
    int32_t rc = m_postprocessor.init(&mMemory, jpegEvtHandle, this);
    if (rc != 0) {
        ALOGE("Init Postprocessor failed");
    }
}

/*===========================================================================
 * FUNCTION   : stop
 *
 * DESCRIPTION: stop pic channel, which will stop all streams within, including
 *              the reprocessing channel in postprocessor and YUV stream.
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3PicChannel::stop()
{
    int32_t rc = NO_ERROR;
    if(!m_bIsActive) {
        ALOGE("%s: Attempt to stop inactive channel",__func__);
        return rc;
    }

    m_postprocessor.stop();

    rc |= QCamera3Channel::stop();
    return rc;
}

QCamera3PicChannel::~QCamera3PicChannel()
{
   stop();

   int32_t rc = m_postprocessor.deinit();
   if (rc != 0) {
       ALOGE("De-init Postprocessor failed");
   }
}

int32_t QCamera3PicChannel::initialize()
{
    int32_t rc = NO_ERROR;
    cam_dimension_t streamDim;
    cam_stream_type_t streamType;
    cam_format_t streamFormat;
    mm_camera_channel_attr_t attr;

    if (NULL == mCamera3Stream) {
        ALOGE("%s: Camera stream uninitialized", __func__);
        return NO_INIT;
    }

    if (1 <= m_numStreams) {
        // Only one stream per channel supported in v3 Hal
        return NO_ERROR;
    }

    memset(&attr, 0, sizeof(mm_camera_channel_attr_t));
    attr.notify_mode = MM_CAMERA_SUPER_BUF_NOTIFY_BURST;
    attr.look_back = 1;
    attr.post_frame_skip = 1;
    attr.water_mark = 1;
    attr.max_unmatched_frames = 1;

    rc = init(&attr, NULL);
    if (rc < 0) {
        ALOGE("%s: init failed", __func__);
        return rc;
    }

    streamType = CAM_STREAM_TYPE_NON_ZSL_SNAPSHOT;
    streamFormat = CAM_FORMAT_YUV_420_NV21;
    streamDim.width = mYuvWidth;
    streamDim.height = mYuvHeight;

    int num_buffers = 1;
    mNumBufs = CAM_MAX_NUM_BUFS_PER_STREAM;
    rc = QCamera3Channel::addStream(streamType, streamFormat, streamDim,
            num_buffers);

    return rc;
}

int32_t QCamera3PicChannel::request(buffer_handle_t *buffer,
        uint32_t frameNumber,
        mm_camera_buf_def_t *pInputBuffer,
        metadata_buffer_t *metadata)
{
    //FIX ME: Return buffer back in case of failures below.

    int32_t rc = NO_ERROR;
    int index;
    // Picture stream has already been started before any request comes in
    if (!m_bIsActive) {
        ALOGE("%s: Channel not started!!", __func__);
        return NO_INIT;
    }

    index = mMemory.getMatchBufIndex((void*)buffer);
    if(index < 0) {
        rc = registerBuffer(buffer);
        if (NO_ERROR != rc) {
            ALOGE("%s: On-the-fly buffer registration failed %d",
                    __func__, rc);
            return rc;
        }

        index = mMemory.getMatchBufIndex((void*)buffer);
        if (index < 0) {
            ALOGE("%s: Could not find object among registered buffers",__func__);
            return DEAD_OBJECT;
        }
    }
    rc = mMemory.markFrameNumber(index, frameNumber);

    //Start the postprocessor for jpeg encoding. Pass mMemory as destination buffer
    mCurrentBufIndex = index;

    // Start postprocessor
    m_postprocessor.start(this, metadata);

    // Queue jpeg settings
    rc = queueJpegSetting(index, metadata);

    if (pInputBuffer == NULL)
        mStreams[0]->bufDone(0);
    else {
        mm_camera_super_buf_t *src_frame = NULL;
        src_frame = (mm_camera_super_buf_t *)malloc(
                sizeof(mm_camera_super_buf_t));
        if (src_frame == NULL) {
            ALOGE("%s: No memory for src frame", __func__);
            return NO_MEMORY;
        }
        memset(src_frame, 0, sizeof(mm_camera_super_buf_t));
        src_frame->num_bufs = 1;
        src_frame->bufs[0] = pInputBuffer;

        ALOGD("%s: Post-process started", __func__);
        ALOGD("%s: Issue call to reprocess", __func__);

        m_postprocessor.processPPMetadata(metadata);
        m_postprocessor.processData(src_frame);
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : dataNotifyCB
 *
 * DESCRIPTION: Channel Level callback used for super buffer data notify.
 *              This function is registered with mm-camera-interface to handle
 *              data notify
 *
 * PARAMETERS :
 *   @recvd_frame   : stream frame received
 *   userdata       : user data ptr
 *
 * RETURN     : none
 *==========================================================================*/
void QCamera3PicChannel::dataNotifyCB(mm_camera_super_buf_t *recvd_frame,
                                 void *userdata)
{
    ALOGV("%s: E\n", __func__);
    QCamera3PicChannel *channel = (QCamera3PicChannel *)userdata;

    if (channel == NULL) {
        ALOGE("%s: invalid channel pointer", __func__);
        return;
    }

    if(channel->m_numStreams != 1) {
        ALOGE("%s: Error: Bug: This callback assumes one stream per channel",__func__);
        return;
    }


    if(channel->mStreams[0] == NULL) {
        ALOGE("%s: Error: Invalid Stream object",__func__);
        return;
    }

    channel->QCamera3PicChannel::streamCbRoutine(recvd_frame, channel->mStreams[0]);

    ALOGV("%s: X\n", __func__);
    return;
}

/*===========================================================================
 * FUNCTION   : registerBuffer
 *
 * DESCRIPTION: register streaming buffer to the channel object
 *
 * PARAMETERS :
 *   @buffer     : buffer to be registered
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3PicChannel::registerBuffer(buffer_handle_t *buffer)
{
    int rc = 0;

    if ((uint32_t)mMemory.getCnt() > (mNumBufs - 1)) {
        ALOGE("%s: Trying to register more buffers than initially requested",
                __func__);
        return BAD_VALUE;
    }

    if (0 == m_numStreams) {
        rc = initialize();
        if (rc != NO_ERROR) {
            ALOGE("%s: Couldn't initialize camera stream %d",
                    __func__, rc);
            return rc;
        }
    }
    rc = mMemory.registerBuffer(buffer);
    if (ALREADY_EXISTS == rc) {
        return NO_ERROR;
    } else if (NO_ERROR != rc) {
        ALOGE("%s: Buffer %p couldn't be registered %d", __func__, buffer, rc);
        return rc;
    }

    return rc;
}

void QCamera3PicChannel::streamCbRoutine(mm_camera_super_buf_t *super_frame,
                            QCamera3Stream *stream)
{
    //TODO
    //Used only for getting YUV. Jpeg callback will be sent back from channel
    //directly to HWI. Refer to func jpegEvtHandle

    //Got the yuv callback. Calling yuv callback handler in PostProc
    uint8_t frameIndex;
    mm_camera_super_buf_t* frame = NULL;
    if(!super_frame) {
         ALOGE("%s: Invalid Super buffer",__func__);
         return;
    }

    if(super_frame->num_bufs != 1) {
         ALOGE("%s: Multiple streams are not supported",__func__);
         return;
    }
    if(super_frame->bufs[0] == NULL ) {
         ALOGE("%s: Error, Super buffer frame does not contain valid buffer",
                  __func__);
         return;
    }

    frameIndex = (uint8_t)super_frame->bufs[0]->buf_idx;
    if(frameIndex >= mNumBufs) {
         ALOGE("%s: Error, Invalid index for buffer",__func__);
         if(stream) {
             stream->bufDone(frameIndex);
         }
         return;
    }

    frame = (mm_camera_super_buf_t *)malloc(sizeof(mm_camera_super_buf_t));
    if (frame == NULL) {
       ALOGE("%s: Error allocating memory to save received_frame structure.",
                                                                    __func__);
       if(stream) {
           stream->bufDone(frameIndex);
       }
       return;
    }
    *frame = *super_frame;
    m_postprocessor.processData(frame);
    free(super_frame);
    return;
}

QCamera3Memory* QCamera3PicChannel::getStreamBufs(uint32_t len)
{
    int rc = 0;

    mYuvMemory = new QCamera3HeapMemory();
    if (!mYuvMemory) {
        ALOGE("%s: unable to create metadata memory", __func__);
        return NULL;
    }

    //Queue YUV buffers in the beginning mQueueAll = true
    rc = mYuvMemory->allocate(1, len, false);
    if (rc < 0) {
        ALOGE("%s: unable to allocate metadata memory", __func__);
        delete mYuvMemory;
        mYuvMemory = NULL;
        return NULL;
    }
    return mYuvMemory;
}

void QCamera3PicChannel::putStreamBufs()
{
    mMemory.unregisterBuffers();

    mYuvMemory->deallocate();
    delete mYuvMemory;
    mYuvMemory = NULL;
}

int32_t QCamera3PicChannel::queueReprocMetadata(metadata_buffer_t *metadata)
{
    return m_postprocessor.processPPMetadata(metadata);
}

int32_t QCamera3PicChannel::queueJpegSetting(int32_t index, metadata_buffer_t *metadata)
{
    jpeg_settings_t *settings =
            (jpeg_settings_t *)malloc(sizeof(jpeg_settings_t));

    if (!settings) {
        ALOGE("%s: out of memory allocating jpeg_settings", __func__);
        return -ENOMEM;
    }

    memset(settings, 0, sizeof(jpeg_settings_t));

    settings->out_buf_index = index;

    settings->jpeg_orientation = 0;
    if (IS_PARM_VALID(CAM_INTF_META_JPEG_ORIENTATION, metadata)) {
        int32_t *orientation = (int32_t *)POINTER_OF(
                CAM_INTF_META_JPEG_ORIENTATION, metadata);
        settings->jpeg_orientation = *orientation;
    }

    settings->jpeg_quality = 85;
    if (IS_PARM_VALID(CAM_INTF_META_JPEG_QUALITY, metadata)) {
        uint8_t *quality = (uint8_t *)POINTER_OF(
                CAM_INTF_META_JPEG_QUALITY, metadata);
        settings->jpeg_quality = *quality;
    }

    if (IS_PARM_VALID(CAM_INTF_META_JPEG_THUMB_QUALITY, metadata)) {
        uint8_t *quality = (uint8_t *)POINTER_OF(
                CAM_INTF_META_JPEG_THUMB_QUALITY, metadata);
        settings->jpeg_thumb_quality = *quality;
    }

    if (IS_PARM_VALID(CAM_INTF_META_JPEG_THUMB_SIZE, metadata)) {
        cam_dimension_t *dimension = (cam_dimension_t *)POINTER_OF(
                CAM_INTF_META_JPEG_THUMB_SIZE, metadata);
        settings->thumbnail_size = *dimension;
    }

    settings->gps_timestamp_valid = 0;
    if (IS_PARM_VALID(CAM_INTF_META_JPEG_GPS_TIMESTAMP, metadata)) {
        int64_t *timestamp = (int64_t *)POINTER_OF(
                CAM_INTF_META_JPEG_GPS_TIMESTAMP, metadata);
        settings->gps_timestamp = *timestamp;
        settings->gps_timestamp_valid = 1;
    }

    settings->gps_coordinates_valid = 0;
    if (IS_PARM_VALID(CAM_INTF_META_JPEG_GPS_COORDINATES, metadata)) {
        double *coordinates = (double *)POINTER_OF(
                CAM_INTF_META_JPEG_GPS_COORDINATES, metadata);
        memcpy(settings->gps_coordinates, coordinates, 3*sizeof(double));
        settings->gps_coordinates_valid = 1;
    }

    if (IS_PARM_VALID(CAM_INTF_META_JPEG_GPS_PROC_METHODS, metadata)) {
        char *proc_methods = (char *)POINTER_OF(
                CAM_INTF_META_JPEG_GPS_PROC_METHODS, metadata);
        memset(settings->gps_processing_method, 0,
                sizeof(settings->gps_processing_method));
        strncpy(settings->gps_processing_method, proc_methods,
                sizeof(settings->gps_processing_method));
    }

    return m_postprocessor.processJpegSettingData(settings);
}

/*===========================================================================
 * FUNCTION   : getRational
 *
 * DESCRIPTION: compose rational struct
 *
 * PARAMETERS :
 *   @rat     : ptr to struct to store rational info
 *   @num     :num of the rational
 *   @denom   : denom of the rational
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t getRational(rat_t *rat, int num, int denom)
{
    if (NULL == rat) {
        ALOGE("%s: NULL rat input", __func__);
        return BAD_VALUE;
    }
    rat->num = num;
    rat->denom = denom;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : parseGPSCoordinate
 *
 * DESCRIPTION: parse GPS coordinate string
 *
 * PARAMETERS :
 *   @coord_str : [input] coordinate string
 *   @coord     : [output]  ptr to struct to store coordinate
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int parseGPSCoordinate(const char *coord_str, rat_t* coord)
{
    if(coord == NULL) {
        ALOGE("%s: error, invalid argument coord == NULL", __func__);
        return BAD_VALUE;
    }
    float degF = atof(coord_str);
    if (degF < 0) {
        degF = -degF;
    }
    float minF = (degF - (int) degF) * 60;
    float secF = (minF - (int) minF) * 60;

    getRational(&coord[0], (int)degF, 1);
    getRational(&coord[1], (int)minF, 1);
    getRational(&coord[2], (int)(secF * 10000), 10000);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getExifDateTime
 *
 * DESCRIPTION: query exif date time
 *
 * PARAMETERS :
 *   @dateTime   : string to store exif date time
 *   @subsecTime : string to store exif subsec time
 *   @count      : length of the dateTime string
 *   @subsecCount: length of the subsecTime string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t getExifDateTime(char *dateTime, char *subsecTime,
        uint32_t &count, uint32_t &subsecCount)
{
    //get time and date from system
    struct timeval tv;
    struct tm *timeinfo;

    gettimeofday(&tv, NULL);
    timeinfo = localtime(&tv.tv_sec);
    //Write datetime according to EXIF Spec
    //"YYYY:MM:DD HH:MM:SS" (20 chars including \0)
    snprintf(dateTime, 20, "%04d:%02d:%02d %02d:%02d:%02d",
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
             timeinfo->tm_mday, timeinfo->tm_hour,
             timeinfo->tm_min, timeinfo->tm_sec);
    count = 20;

    //Write subsec according to EXIF Sepc
    snprintf(subsecTime, 7, "%06ld", tv.tv_usec);
    subsecCount = 7;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getExifFocalLength
 *
 * DESCRIPTION: get exif focal lenght
 *
 * PARAMETERS :
 *   @focalLength : ptr to rational strcut to store focal lenght
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t getExifFocalLength(rat_t *focalLength, float value)
{
    int focalLengthValue =
        (int)(value * FOCAL_LENGTH_DECIMAL_PRECISION);
    return getRational(focalLength, focalLengthValue, FOCAL_LENGTH_DECIMAL_PRECISION);
}

/*===========================================================================
  * FUNCTION   : getExifExpTimeInfo
  *
  * DESCRIPTION: get exif exposure time information
  *
  * PARAMETERS :
  *   @expoTimeInfo     : expousure time value
  * RETURN     : nt32_t type of status
  *              NO_ERROR  -- success
  *              none-zero failure code
  *==========================================================================*/
int32_t getExifExpTimeInfo(rat_t *expoTimeInfo, int64_t value)
{

    int cal_exposureTime;
    if (value != 0)
        cal_exposureTime = value;
    else
        cal_exposureTime = 60;

    return getRational(expoTimeInfo, 1, cal_exposureTime);
}

/*===========================================================================
 * FUNCTION   : getExifGpsProcessingMethod
 *
 * DESCRIPTION: get GPS processing method
 *
 * PARAMETERS :
 *   @gpsProcessingMethod : string to store GPS process method
 *   @count               : lenght of the string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t getExifGpsProcessingMethod(char *gpsProcessingMethod,
                                   uint32_t &count, char* value)
{
    if(value != NULL) {
        memcpy(gpsProcessingMethod, ExifAsciiPrefix, EXIF_ASCII_PREFIX_SIZE);
        count = EXIF_ASCII_PREFIX_SIZE;
        strncpy(gpsProcessingMethod + EXIF_ASCII_PREFIX_SIZE, value, strlen(value));
        count += strlen(value);
        gpsProcessingMethod[count++] = '\0'; // increase 1 for the last NULL char
        return NO_ERROR;
    } else {
        return BAD_VALUE;
    }
}

/*===========================================================================
 * FUNCTION   : getExifLatitude
 *
 * DESCRIPTION: get exif latitude
 *
 * PARAMETERS :
 *   @latitude : ptr to rational struct to store latitude info
 *   @ladRef   : charater to indicate latitude reference
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t getExifLatitude(rat_t *latitude,
                                           char *latRef, double value)
{
    char str[30];
    snprintf(str, sizeof(str), "%f", value);
    if(str != NULL) {
        parseGPSCoordinate(str, latitude);

        //set Latitude Ref
        float latitudeValue = strtof(str, 0);
        if(latitudeValue < 0.0f) {
            latRef[0] = 'S';
        } else {
            latRef[0] = 'N';
        }
        latRef[1] = '\0';
        return NO_ERROR;
    }else{
        return BAD_VALUE;
    }
}

/*===========================================================================
 * FUNCTION   : getExifLongitude
 *
 * DESCRIPTION: get exif longitude
 *
 * PARAMETERS :
 *   @longitude : ptr to rational struct to store longitude info
 *   @lonRef    : charater to indicate longitude reference
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t getExifLongitude(rat_t *longitude,
                                            char *lonRef, double value)
{
    char str[30];
    snprintf(str, sizeof(str), "%f", value);
    if(str != NULL) {
        parseGPSCoordinate(str, longitude);

        //set Longitude Ref
        float longitudeValue = strtof(str, 0);
        if(longitudeValue < 0.0f) {
            lonRef[0] = 'W';
        } else {
            lonRef[0] = 'E';
        }
        lonRef[1] = '\0';
        return NO_ERROR;
    }else{
        return BAD_VALUE;
    }
}

/*===========================================================================
 * FUNCTION   : getExifAltitude
 *
 * DESCRIPTION: get exif altitude
 *
 * PARAMETERS :
 *   @altitude : ptr to rational struct to store altitude info
 *   @altRef   : charater to indicate altitude reference
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t getExifAltitude(rat_t *altitude,
                                           char *altRef, double value)
{
    char str[30];
    snprintf(str, sizeof(str), "%f", value);
    if(str != NULL) {
        double value = atof(str);
        *altRef = 0;
        if(value < 0){
            *altRef = 1;
            value = -value;
        }
        return getRational(altitude, value*1000, 1000);
    }else{
        return BAD_VALUE;
    }
}

/*===========================================================================
 * FUNCTION   : getExifGpsDateTimeStamp
 *
 * DESCRIPTION: get exif GPS date time stamp
 *
 * PARAMETERS :
 *   @gpsDateStamp : GPS date time stamp string
 *   @bufLen       : length of the string
 *   @gpsTimeStamp : ptr to rational struct to store time stamp info
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t getExifGpsDateTimeStamp(char *gpsDateStamp,
                                           uint32_t bufLen,
                                           rat_t *gpsTimeStamp, int64_t value)
{
    char str[30];
    snprintf(str, sizeof(str), "%lld", value);
    if(str != NULL) {
        time_t unixTime = (time_t)atol(str);
        struct tm *UTCTimestamp = gmtime(&unixTime);

        strftime(gpsDateStamp, bufLen, "%Y:%m:%d", UTCTimestamp);

        getRational(&gpsTimeStamp[0], UTCTimestamp->tm_hour, 1);
        getRational(&gpsTimeStamp[1], UTCTimestamp->tm_min, 1);
        getRational(&gpsTimeStamp[2], UTCTimestamp->tm_sec, 1);

        return NO_ERROR;
    } else {
        return BAD_VALUE;
    }
}

int32_t getExifExposureValue(srat_t* exposure_val, int32_t exposure_comp,
                             cam_rational_type_t step)
{
    exposure_val->num = exposure_comp * step.numerator;
    exposure_val->denom = step.denominator;
    return 0;
}
/*===========================================================================
 * FUNCTION   : getExifData
 *
 * DESCRIPTION: get exif data to be passed into jpeg encoding
 *
 * PARAMETERS : none
 *
 * RETURN     : exif data from user setting and GPS
 *==========================================================================*/
QCamera3Exif *QCamera3PicChannel::getExifData(metadata_buffer_t *metadata,
        jpeg_settings_t *jpeg_settings)
{
    QCamera3Exif *exif = new QCamera3Exif();
    if (exif == NULL) {
        ALOGE("%s: No memory for QCamera3Exif", __func__);
        return NULL;
    }

    int32_t rc = NO_ERROR;
    uint32_t count = 0;

    // add exif entries
    {
        char dateTime[20];
        char subsecTime[7];
        uint32_t subsecCount;
        memset(dateTime, 0, sizeof(dateTime));
        memset(subsecTime, 0, sizeof(subsecTime));
        count = 20;
        subsecCount = 7;
        rc = getExifDateTime(dateTime, subsecTime, count, subsecCount);
        if(rc == NO_ERROR) {
            exif->addEntry(EXIFTAGID_DATE_TIME,
                    EXIF_ASCII,
                    count,
                    (void *)dateTime);
            exif->addEntry(EXIFTAGID_EXIF_DATE_TIME_ORIGINAL,
                    EXIF_ASCII,
                    count,
                    (void *)dateTime);
            exif->addEntry(EXIFTAGID_EXIF_DATE_TIME_DIGITIZED,
                    EXIF_ASCII,
                    count,
                    (void *)dateTime);
            exif->addEntry(EXIFTAGID_SUBSEC_TIME,
                    EXIF_ASCII,
                    subsecCount,
                    (void *)subsecTime);
            exif->addEntry(EXIFTAGID_SUBSEC_TIME_ORIGINAL,
                    EXIF_ASCII,
                    subsecCount,
                    (void *)subsecTime);
            exif->addEntry(EXIFTAGID_SUBSEC_TIME_DIGITIZED,
                    EXIF_ASCII,
                    subsecCount,
                    (void *)subsecTime);
        } else {
            ALOGE("%s: getExifDateTime failed", __func__);
        }
    }

    if (IS_PARM_VALID(CAM_INTF_META_LENS_FOCAL_LENGTH, metadata)) {
        float focal_length = *(float *)POINTER_OF(
                CAM_INTF_META_LENS_FOCAL_LENGTH, metadata);
        rat_t focalLength;
        rc = getExifFocalLength(&focalLength, focal_length);
        if (rc == NO_ERROR) {
            exif->addEntry(EXIFTAGID_FOCAL_LENGTH,
                    EXIF_RATIONAL,
                    1,
                    (void *)&(focalLength));
        } else {
            ALOGE("%s: getExifFocalLength failed", __func__);
        }
    }

    if (IS_PARM_VALID(CAM_INTF_META_SENSOR_SENSITIVITY, metadata)) {
        int16_t isoSpeed = *(int32_t *)POINTER_OF(
                CAM_INTF_META_SENSOR_SENSITIVITY, metadata);
        exif->addEntry(EXIFTAGID_ISO_SPEED_RATING,
                   EXIF_SHORT,
                   1,
                   (void *)&(isoSpeed));
    }

    if (IS_PARM_VALID(CAM_INTF_META_SENSOR_EXPOSURE_TIME, metadata)) {
        int64_t sensor_exposure_time = *(int64_t *)POINTER_OF(
                CAM_INTF_META_SENSOR_EXPOSURE_TIME, metadata);
        rat_t sensorExpTime;
        rc = getExifExpTimeInfo(&sensorExpTime, sensor_exposure_time);
        if (rc == NO_ERROR){
            exif->addEntry(EXIFTAGID_EXPOSURE_TIME,
                    EXIF_RATIONAL,
                    1,
                    (void *)&(sensorExpTime));
        } else {
            ALOGE("%s: getExifExpTimeInfo failed", __func__);
        }
    }

    if (strlen(jpeg_settings->gps_processing_method) > 0) {
        char gpsProcessingMethod[
                    EXIF_ASCII_PREFIX_SIZE + GPS_PROCESSING_METHOD_SIZE];
        count = 0;
        rc = getExifGpsProcessingMethod(gpsProcessingMethod,
                count, jpeg_settings->gps_processing_method);
        if(rc == NO_ERROR) {
            exif->addEntry(EXIFTAGID_GPS_PROCESSINGMETHOD,
                    EXIF_ASCII,
                    count,
                    (void *)gpsProcessingMethod);
        } else {
            ALOGE("%s: getExifGpsProcessingMethod failed", __func__);
        }
    }

    if (jpeg_settings->gps_coordinates_valid) {

        //latitude
        rat_t latitude[3];
        char latRef[2];
        rc = getExifLatitude(latitude, latRef,
                jpeg_settings->gps_coordinates[0]);
        if(rc == NO_ERROR) {
            exif->addEntry(EXIFTAGID_GPS_LATITUDE,
                           EXIF_RATIONAL,
                           3,
                           (void *)latitude);
            exif->addEntry(EXIFTAGID_GPS_LATITUDE_REF,
                           EXIF_ASCII,
                           2,
                           (void *)latRef);
        } else {
            ALOGE("%s: getExifLatitude failed", __func__);
        }

        //longitude
        rat_t longitude[3];
        char lonRef[2];
        rc = getExifLongitude(longitude, lonRef,
                jpeg_settings->gps_coordinates[1]);
        if(rc == NO_ERROR) {
            exif->addEntry(EXIFTAGID_GPS_LONGITUDE,
                           EXIF_RATIONAL,
                           3,
                           (void *)longitude);

            exif->addEntry(EXIFTAGID_GPS_LONGITUDE_REF,
                           EXIF_ASCII,
                           2,
                           (void *)lonRef);
        } else {
            ALOGE("%s: getExifLongitude failed", __func__);
        }

        //altitude
        rat_t altitude;
        char altRef;
        rc = getExifAltitude(&altitude, &altRef,
                jpeg_settings->gps_coordinates[2]);
        if(rc == NO_ERROR) {
            exif->addEntry(EXIFTAGID_GPS_ALTITUDE,
                           EXIF_RATIONAL,
                           1,
                           (void *)&(altitude));

            exif->addEntry(EXIFTAGID_GPS_ALTITUDE_REF,
                           EXIF_BYTE,
                           1,
                           (void *)&altRef);
        } else {
            ALOGE("%s: getExifAltitude failed", __func__);
        }
    }

    if (jpeg_settings->gps_timestamp_valid) {

        char gpsDateStamp[20];
        rat_t gpsTimeStamp[3];
        rc = getExifGpsDateTimeStamp(gpsDateStamp, 20, gpsTimeStamp,
                jpeg_settings->gps_timestamp);
        if(rc == NO_ERROR) {
            exif->addEntry(EXIFTAGID_GPS_DATESTAMP,
                           EXIF_ASCII,
                           strlen(gpsDateStamp) + 1,
                           (void *)gpsDateStamp);

            exif->addEntry(EXIFTAGID_GPS_TIMESTAMP,
                           EXIF_RATIONAL,
                           3,
                           (void *)gpsTimeStamp);
        } else {
            ALOGE("%s: getExifGpsDataTimeStamp failed", __func__);
        }
    }

    if (IS_PARM_VALID(CAM_INTF_PARM_EV, metadata) &&
            IS_PARM_VALID(CAM_INTF_PARM_EV_STEP, metadata)) {
        int32_t exposure_comp = *(int32_t *)POINTER_OF(
                CAM_INTF_PARM_EV, metadata);
        cam_rational_type_t comp_step = *(cam_rational_type_t *)POINTER_OF(
                CAM_INTF_PARM_EV_STEP, metadata);
        srat_t exposure_val;
        rc = getExifExposureValue(&exposure_val, exposure_comp, comp_step);
        if(rc == NO_ERROR) {
            exif->addEntry(EXIFTAGID_EXPOSURE_BIAS_VALUE,
                       EXIF_SRATIONAL,
                       1,
                       (void *)(&exposure_val));
        } else {
            ALOGE("%s: getExifExposureValue failed ", __func__);
        }
    }

    char value[PROPERTY_VALUE_MAX];
    if (property_get("ro.product.manufacturer", value, "QCOM-AA") > 0) {
        exif->addEntry(EXIFTAGID_MAKE,
                       EXIF_ASCII,
                       strlen(value) + 1,
                       (void *)value);
    } else {
        ALOGE("%s: getExifMaker failed", __func__);
    }

    if (property_get("ro.product.model", value, "QCAM-AA") > 0) {
        exif->addEntry(EXIFTAGID_MODEL,
                       EXIF_ASCII,
                       strlen(value) + 1,
                       (void *)value);
    } else {
        ALOGE("%s: getExifModel failed", __func__);
    }

    return exif;
}

void QCamera3PicChannel::overrideYuvSize(uint32_t width, uint32_t height)
{
   mYuvWidth = width;
   mYuvHeight = height;
}

int QCamera3PicChannel::kMaxBuffers = 1;

/*===========================================================================
 * FUNCTION   : QCamera3ReprocessChannel
 *
 * DESCRIPTION: constructor of QCamera3ReprocessChannel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *   @pp_mask    : post-proccess feature mask
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3ReprocessChannel::QCamera3ReprocessChannel(uint32_t cam_handle,
                                                 mm_camera_ops_t *cam_ops,
                                                 channel_cb_routine cb_routine,
                                                 cam_padding_info_t *paddingInfo,
                                                 void *userData, void *ch_hdl) :
    QCamera3Channel(cam_handle, cam_ops, cb_routine, paddingInfo, userData),
    picChHandle(ch_hdl),
    m_pSrcChannel(NULL),
    m_pMetaChannel(NULL),
    mMemory(NULL)
{
    memset(mSrcStreamHandles, 0, sizeof(mSrcStreamHandles));
}


/*===========================================================================
 * FUNCTION   : QCamera3ReprocessChannel
 *
 * DESCRIPTION: constructor of QCamera3ReprocessChannel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *   @pp_mask    : post-proccess feature mask
 *
 * RETURN     : none
 *==========================================================================*/
int32_t QCamera3ReprocessChannel::initialize()
{
    int32_t rc = NO_ERROR;
    mm_camera_channel_attr_t attr;

    memset(&attr, 0, sizeof(mm_camera_channel_attr_t));
    attr.notify_mode = MM_CAMERA_SUPER_BUF_NOTIFY_CONTINUOUS;
    attr.max_unmatched_frames = 1;

    rc = init(&attr, NULL);
    if (rc < 0) {
        ALOGE("%s: init failed", __func__);
    }
    return rc;
}


/*===========================================================================
 * FUNCTION   : QCamera3ReprocessChannel
 *
 * DESCRIPTION: constructor of QCamera3ReprocessChannel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *   @pp_mask    : post-proccess feature mask
 *
 * RETURN     : none
 *==========================================================================*/
void QCamera3ReprocessChannel::streamCbRoutine(mm_camera_super_buf_t *super_frame,
                                  QCamera3Stream *stream)
{
    //Got the pproc data callback. Now send to jpeg encoding
    uint8_t frameIndex;
    mm_camera_super_buf_t* frame = NULL;
    QCamera3PicChannel *obj = (QCamera3PicChannel *)picChHandle;

    if(!super_frame) {
         ALOGE("%s: Invalid Super buffer",__func__);
         return;
    }

    if(super_frame->num_bufs != 1) {
         ALOGE("%s: Multiple streams are not supported",__func__);
         return;
    }
    if(super_frame->bufs[0] == NULL ) {
         ALOGE("%s: Error, Super buffer frame does not contain valid buffer",
                  __func__);
         return;
    }

    frameIndex = (uint8_t)super_frame->bufs[0]->buf_idx;
    frame = (mm_camera_super_buf_t *)malloc(sizeof(mm_camera_super_buf_t));
    if (frame == NULL) {
       ALOGE("%s: Error allocating memory to save received_frame structure.",
                                                                    __func__);
       if(stream) {
           stream->bufDone(frameIndex);
       }
       return;
    }
    *frame = *super_frame;
    obj->m_postprocessor.processPPData(frame);
    free(super_frame);
    return;
}

/*===========================================================================
 * FUNCTION   : QCamera3ReprocessChannel
 *
 * DESCRIPTION: default constructor of QCamera3ReprocessChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3ReprocessChannel::QCamera3ReprocessChannel() :
    m_pSrcChannel(NULL),
    m_pMetaChannel(NULL)
{
}

/*===========================================================================
 * FUNCTION   : getStreamBufs
 *
 * DESCRIPTION: register the buffers of the reprocess channel
 *
 * PARAMETERS : none
 *
 * RETURN     : QCamera3Memory *
 *==========================================================================*/
QCamera3Memory* QCamera3ReprocessChannel::getStreamBufs(uint32_t len)
{
   int rc = 0;

    mMemory = new QCamera3HeapMemory();
    if (!mMemory) {
        ALOGE("%s: unable to create reproc memory", __func__);
        return NULL;
    }

    //Queue YUV buffers in the beginning mQueueAll = true
    rc = mMemory->allocate(2, len, true);
    if (rc < 0) {
        ALOGE("%s: unable to allocate reproc memory", __func__);
        delete mMemory;
        mMemory = NULL;
        return NULL;
    }
    return mMemory;
}

/*===========================================================================
 * FUNCTION   : getStreamBufs
 *
 * DESCRIPTION: register the buffers of the reprocess channel
 *
 * PARAMETERS : none
 *
 * RETURN     :
 *==========================================================================*/
void QCamera3ReprocessChannel::putStreamBufs()
{
    mMemory->deallocate();
    delete mMemory;
    mMemory = NULL;
}

/*===========================================================================
 * FUNCTION   : ~QCamera3ReprocessChannel
 *
 * DESCRIPTION: destructor of QCamera3ReprocessChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3ReprocessChannel::~QCamera3ReprocessChannel()
{
}

/*===========================================================================
 * FUNCTION   : getStreamBySrcHandle
 *
 * DESCRIPTION: find reprocess stream by its source stream handle
 *
 * PARAMETERS :
 *   @srcHandle : source stream handle
 *
 * RETURN     : ptr to reprocess stream if found. NULL if not found
 *==========================================================================*/
QCamera3Stream * QCamera3ReprocessChannel::getStreamBySrcHandle(uint32_t srcHandle)
{
    QCamera3Stream *pStream = NULL;

    for (int i = 0; i < m_numStreams; i++) {
        if (mSrcStreamHandles[i] == srcHandle) {
            pStream = mStreams[i];
            break;
        }
    }
    return pStream;
}

/*===========================================================================
 * FUNCTION   : getSrcStreamBySrcHandle
 *
 * DESCRIPTION: find source stream by source stream handle
 *
 * PARAMETERS :
 *   @srcHandle : source stream handle
 *
 * RETURN     : ptr to reprocess stream if found. NULL if not found
 *==========================================================================*/
QCamera3Stream * QCamera3ReprocessChannel::getSrcStreamBySrcHandle(uint32_t srcHandle)
{
    QCamera3Stream *pStream = NULL;

    for (int i = 0; i < m_numStreams; i++) {
        if (mSrcStreamHandles[i] == srcHandle) {
            pStream = m_pSrcChannel->getStreamByIndex(i);
            break;
        }
    }
    return pStream;
}

/*===========================================================================
 * FUNCTION   : metadataBufDone
 *
 * DESCRIPTION: buf done method for a metadata buffer
 *
 * PARAMETERS :
 *   @recvd_frame : received metadata frame
 *
 * RETURN     :
 *==========================================================================*/
int32_t QCamera3ReprocessChannel::metadataBufDone(mm_camera_super_buf_t *recvd_frame)
{
   int32_t rc;
   rc = ((QCamera3MetadataChannel*)m_pMetaChannel)->bufDone(recvd_frame);
   free(recvd_frame);
   recvd_frame = NULL;
   return rc;
}

/*===========================================================================
 * FUNCTION   : doReprocess
 *
 * DESCRIPTION: request to do a reprocess on the frame
 *
 * PARAMETERS :
 *   @frame   : frame to be performed a reprocess
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3ReprocessChannel::doReprocess(mm_camera_super_buf_t *frame,
                                              mm_camera_super_buf_t *meta_frame)
{
    int32_t rc = 0;
    if (m_numStreams < 1) {
        ALOGE("%s: No reprocess stream is created", __func__);
        return -1;
    }
    if (m_pSrcChannel == NULL) {
        ALOGE("%s: No source channel for reprocess", __func__);
        return -1;
    }
    for (int i = 0; i < frame->num_bufs; i++) {
        QCamera3Stream *pStream = getStreamBySrcHandle(frame->bufs[i]->stream_id);
        if (pStream != NULL) {
            cam_stream_parm_buffer_t param;
            memset(&param, 0, sizeof(cam_stream_parm_buffer_t));
            param.type = CAM_STREAM_PARAM_TYPE_DO_REPROCESS;
            param.reprocess.buf_index = frame->bufs[i]->buf_idx;
            if (meta_frame != NULL) {
               param.reprocess.meta_present = 1;
               param.reprocess.meta_stream_handle = m_pMetaChannel->mStreams[0]->getMyServerID();
               param.reprocess.meta_buf_index = meta_frame->bufs[0]->buf_idx;
            }
            rc = pStream->setParameter(param);
            if (rc != NO_ERROR) {
                ALOGE("%s: stream setParameter for reprocess failed", __func__);
                break;
            }
        }
    }
    return rc;
}

int32_t QCamera3ReprocessChannel::doReprocessOffline(mm_camera_super_buf_t *frame,
        metadata_buffer_t *metadata)
{
    int32_t rc = 0;
    OfflineBuffer mappedBuffer;
    if (m_numStreams < 1) {
        ALOGE("%s: No reprocess stream is created", __func__);
        return -1;
    }
    if (m_pSrcChannel == NULL) {
        ALOGE("%s: No source channel for reprocess", __func__);
        return -1;
    }

    uint32_t buf_idx = 0;
    for (int i = 0; i < frame->num_bufs; i++) {
        QCamera3Stream *pStream = getStreamBySrcHandle(frame->bufs[i]->stream_id);
        QCamera3Stream *pSrcStream = getSrcStreamBySrcHandle(frame->bufs[i]->stream_id);
        if (pStream != NULL && pSrcStream != NULL) {

            rc = mStreams[i]->mapBuf(
                    CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF,
                    buf_idx, -1,
                    frame->bufs[i]->fd, frame->bufs[i]->frame_len);

            if (rc == NO_ERROR) {
                memset(&mappedBuffer, 0, sizeof(OfflineBuffer));
                mappedBuffer.index = buf_idx;
                mappedBuffer.stream = pStream;
                mappedBuffer.type = CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF;
                mOfflineBuffers.push_back(mappedBuffer);

                cam_stream_parm_buffer_t param;
                memset(&param, 0, sizeof(cam_stream_parm_buffer_t));
                param.type = CAM_STREAM_PARAM_TYPE_DO_REPROCESS;
                param.reprocess.buf_index = buf_idx;

                param.reprocess.meta_present = 1;
                char* private_data = (char *)POINTER_OF(
                        CAM_INTF_META_PRIVATE_DATA, metadata);
                memcpy(param.reprocess.private_data, private_data,
                        MAX_METADATA_PAYLOAD_SIZE);

                // Find crop info for reprocess stream
                cam_crop_data_t *crop_data = (cam_crop_data_t *)
                        POINTER_OF(CAM_INTF_META_CROP_DATA, metadata);
                for (int j = 0; j < crop_data->num_of_streams; j++) {
                    if (crop_data->crop_info[j].stream_id ==
                           pSrcStream->getMyServerID()) {
                        param.reprocess.crop_rect  =
                                crop_data->crop_info[j].crop;
                        break;
                    }
                }
                rc = pStream->setParameter(param);
                if (rc != NO_ERROR) {
                    ALOGE("%s: stream setParameter for reprocess failed", __func__);
                    break;
                }
            }
        }
    }
    return rc;
}

/*===========================================================================
* FUNCTION : stop
*
* DESCRIPTION: Unmap offline buffers and stop channel
*
* PARAMETERS : none
*
* RETURN : int32_t type of status
* NO_ERROR -- success
* none-zero failure code
*==========================================================================*/
int32_t QCamera3ReprocessChannel::stop()
{
    if (!mOfflineBuffers.empty()) {
        QCamera3Stream *stream = NULL;
        List<OfflineBuffer>::iterator it = mOfflineBuffers.begin();
        int error = NO_ERROR;
        for( ; it != mOfflineBuffers.end(); it++) {
            stream = (*it).stream;
            if (NULL != stream) {
                error = stream->unmapBuf((*it).type,
                        (*it).index,
                        -1);
                if (NO_ERROR != error) {
                    ALOGE("%s: Error during offline buffer unmap %d",
                            __func__, error);
                }
            }
        }
        mOfflineBuffers.clear();
    }

    return QCamera3Channel::stop();
}

/*===========================================================================
 * FUNCTION   : doReprocess
 *
 * DESCRIPTION: request to do a reprocess on the frame
 *
 * PARAMETERS :
 *   @buf_fd     : fd to the input buffer that needs reprocess
 *   @buf_lenght : length of the input buffer
 *   @ret_val    : result of reprocess.
 *                 Example: Could be faceID in case of register face image.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3ReprocessChannel::doReprocess(int buf_fd,
                                              uint32_t buf_length,
                                              int32_t &ret_val,
                                              mm_camera_super_buf_t *meta_frame)
{
    int32_t rc = 0;
    if (m_numStreams < 1) {
        ALOGE("%s: No reprocess stream is created", __func__);
        return -1;
    }
    if (meta_frame == NULL) {
        ALOGE("%s: Did not get corresponding metadata in time", __func__);
        return -1;
    }

    uint32_t buf_idx = 0;
    for (int i = 0; i < m_numStreams; i++) {
        rc = mStreams[i]->mapBuf(CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF,
                                 buf_idx, -1,
                                 buf_fd, buf_length);

        if (rc == NO_ERROR) {
            cam_stream_parm_buffer_t param;
            memset(&param, 0, sizeof(cam_stream_parm_buffer_t));
            param.type = CAM_STREAM_PARAM_TYPE_DO_REPROCESS;
            param.reprocess.buf_index = buf_idx;
            param.reprocess.meta_present = 1;
            param.reprocess.meta_stream_handle = m_pMetaChannel->mStreams[0]->getMyServerID();
            param.reprocess.meta_buf_index = meta_frame->bufs[0]->buf_idx;
            rc = mStreams[i]->setParameter(param);
            if (rc == NO_ERROR) {
                ret_val = param.reprocess.ret_val;
            }
            mStreams[i]->unmapBuf(CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF,
                                  buf_idx, -1);
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : addReprocStreamsFromSource
 *
 * DESCRIPTION: add reprocess streams from input source channel
 *
 * PARAMETERS :
 *   @config         : pp feature configuration
 *   @pSrcChannel    : ptr to input source channel that needs reprocess
 *   @pMetaChannel   : ptr to metadata channel to get corresp. metadata
 *   @offline        : configure for offline reprocessing
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3ReprocessChannel::addReprocStreamsFromSource(cam_pp_feature_config_t &pp_config,
                                                             QCamera3Channel *pSrcChannel,
                                                             QCamera3Channel *pMetaChannel)
{
    int32_t rc = 0;
    QCamera3Stream *pSrcStream = pSrcChannel->getStreamByIndex(0);
    if (pSrcStream == NULL) {
       ALOGE("%s: source channel doesn't have a stream", __func__);
       return BAD_VALUE;
    }
    cam_stream_reproc_config_t reprocess_config;
    cam_dimension_t streamDim;
    cam_stream_type_t streamType;
    cam_format_t streamFormat;
    cam_frame_len_offset_t frameOffset;
    int num_buffers = 2;

    streamType = CAM_STREAM_TYPE_OFFLINE_PROC;
    pSrcStream->getFormat(streamFormat);
    pSrcStream->getFrameDimension(streamDim);
    pSrcStream->getFrameOffset(frameOffset);
    reprocess_config.pp_type = CAM_OFFLINE_REPROCESS_TYPE;

    reprocess_config.offline.input_fmt = streamFormat;
    reprocess_config.offline.input_dim = streamDim;
    reprocess_config.offline.input_buf_planes.plane_info = frameOffset;
    reprocess_config.offline.num_of_bufs = num_buffers;
    reprocess_config.offline.input_stream_type = pSrcStream->getMyType();


    reprocess_config.pp_feature_config = pp_config;
    mSrcStreamHandles[m_numStreams] = pSrcStream->getMyHandle();

    // pp feature config
    if (pp_config.feature_mask & CAM_QCOM_FEATURE_ROTATION) {
        if (pp_config.rotation == ROTATE_90 ||
            pp_config.rotation == ROTATE_270) {
            // rotated by 90 or 270, need to switch width and height
            int32_t temp = streamDim.height;
            streamDim.height = streamDim.width;
            streamDim.width = temp;
        }
    }

    QCamera3Stream *pStream = new QCamera3Stream(m_camHandle,
                                               m_handle,
                                               m_camOps,
                                               mPaddingInfo,
                                               (QCamera3Channel*)this);
    if (pStream == NULL) {
        ALOGE("%s: No mem for Stream", __func__);
        return NO_MEMORY;
    }

    rc = pStream->init(streamType, streamFormat, streamDim, &reprocess_config,
                       num_buffers,QCamera3Channel::streamCbRoutine, this);


    if (rc == 0) {
        mStreams[m_numStreams] = pStream;
        m_numStreams++;
    } else {
        ALOGE("%s: failed to create reprocess stream", __func__);
        delete pStream;
    }

    if (rc == NO_ERROR) {
        m_pSrcChannel = pSrcChannel;
        m_pMetaChannel = pMetaChannel;
    }
    if(m_camOps->request_super_buf(m_camHandle,m_handle,1) < 0) {
        ALOGE("%s: Request for super buffer failed",__func__);
    }
    return rc;
}

cam_dimension_t QCamera3SupportChannel::kDim = {640, 480};

QCamera3SupportChannel::QCamera3SupportChannel(uint32_t cam_handle,
                    mm_camera_ops_t *cam_ops,
                    cam_padding_info_t *paddingInfo,
                    void *userData) :
                        QCamera3Channel(cam_handle, cam_ops,
                                NULL, paddingInfo, userData),
                        mMemory(NULL)
{
}

QCamera3SupportChannel::~QCamera3SupportChannel()
{
    if (m_bIsActive)
        stop();

    if (mMemory) {
        mMemory->deallocate();
        delete mMemory;
        mMemory = NULL;
    }
}

int32_t QCamera3SupportChannel::initialize()
{
    int32_t rc;

    if (mMemory || m_numStreams > 0) {
        ALOGE("%s: Support channel already initialized", __func__);
        return -EINVAL;
    }

    rc = init(NULL, NULL);
    if (rc < 0) {
        ALOGE("%s: init failed", __func__);
        return rc;
    }

    // Hardcode to VGA size for now
    rc = QCamera3Channel::addStream(CAM_STREAM_TYPE_CALLBACK,
        CAM_FORMAT_YUV_420_NV21, kDim, MIN_STREAMING_BUFFER_NUM);
    if (rc < 0) {
        ALOGE("%s: addStream failed", __func__);
    }
    return rc;
}

int32_t QCamera3SupportChannel::request(buffer_handle_t * /*buffer*/,
                                                uint32_t /*frameNumber*/)
{
    return NO_ERROR;
}

void QCamera3SupportChannel::streamCbRoutine(
                        mm_camera_super_buf_t *super_frame,
                        QCamera3Stream * /*stream*/)
{
    if (super_frame == NULL || super_frame->num_bufs != 1) {
        ALOGE("%s: super_frame is not valid", __func__);
        return;
    }
    bufDone(super_frame);
    free(super_frame);
}

QCamera3Memory* QCamera3SupportChannel::getStreamBufs(uint32_t len)
{
    int rc;

    mMemory = new QCamera3HeapMemory();
    if (!mMemory) {
        ALOGE("%s: unable to create heap memory", __func__);
        return NULL;
    }
    rc = mMemory->allocate(MIN_STREAMING_BUFFER_NUM, len, true);
    if (rc < 0) {
        ALOGE("%s: unable to allocate heap memory", __func__);
        delete mMemory;
        mMemory = NULL;
        return NULL;
    }
    return mMemory;
}

void QCamera3SupportChannel::putStreamBufs()
{
    mMemory->deallocate();
    delete mMemory;
    mMemory = NULL;
}

}; // namespace qcamera
