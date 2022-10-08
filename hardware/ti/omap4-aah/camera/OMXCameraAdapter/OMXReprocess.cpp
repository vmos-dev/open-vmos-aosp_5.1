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
* @file OMXReprocess.cpp
*
* This file contains functionality for handling reprocessing operations.
*
*/

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"


namespace Ti {
namespace Camera {

status_t OMXCameraAdapter::setParametersReprocess(const android::CameraParameters &params,
                                                CameraBuffer* buffers,
                                                BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    int w, h, s;
    OMX_COLOR_FORMATTYPE pixFormat;
    OMXCameraPortParameters *portData;
    const char* valstr;

    LOG_FUNCTION_NAME;

    if (!buffers) {
        CAMHAL_LOGE("invalid buffer array");
        return BAD_VALUE;
    }

    portData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoInPortIndex];

    w = buffers[0].width;
    h = buffers[0].height;
    s = buffers[0].stride;

    valstr = buffers[0].format;
    if (valstr != NULL) {
        if(strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
            CAMHAL_LOGDA("YUV420SP format selected");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        } else if (strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB) == 0) {
            CAMHAL_LOGDA("RAW Picture format selected");
            pixFormat = OMX_COLOR_FormatRawBayer10bit;
        } else if (strcmp(valstr, android::CameraParameters::PIXEL_FORMAT_YUV422I) == 0) {
            CAMHAL_LOGDA("YUV422i Picture format selected");
            pixFormat = OMX_COLOR_FormatCbYCrY;
        } else {
            CAMHAL_LOGDA("Format not supported, selecting YUV420SP by default");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        }
    } else {
        CAMHAL_LOGDA("Format not supported, selecting YUV420SP by default");
        pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    }

    if ( (w != (int)portData->mWidth) || (h != (int)portData->mHeight) ||
         (s != (int) portData->mStride) || (pixFormat != portData->mColorFormat)) {
        portData->mWidth = w;
        portData->mHeight = h;

        if ( ( OMX_COLOR_FormatRawBayer10bit == pixFormat ) ||
             ( OMX_COLOR_FormatCbYCrY == pixFormat ) ) {
            portData->mStride = w * 2;
        } else {
            portData->mStride = s;
        }

        portData->mColorFormat = pixFormat;

        mPendingReprocessSettings |= SetFormat;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::startReprocess()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * portData = NULL;

    LOG_FUNCTION_NAME;
    CAMHAL_LOGD ("mReprocConfigured = %d", mReprocConfigured);
    if (!mReprocConfigured) {
        return NO_ERROR;
    }

    portData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoInPortIndex];

    CAMHAL_LOGD ("mReprocConfigured = %d", mBurstFramesQueued);
    if (NO_ERROR == ret) {
        android::AutoMutex lock(mBurstLock);

        for ( int index = 0 ; index < portData->mMaxQueueable ; index++ ) {
            CAMHAL_LOGDB("Queuing buffer on video input port - %p, offset: %d, length: %d",
                         portData->mBufferHeader[index]->pBuffer,
                         portData->mBufferHeader[index]->nOffset,
                         portData->mBufferHeader[index]->nFilledLen);
            portData->mStatus[index] = OMXCameraPortParameters::FILL;
            eError = OMX_EmptyThisBuffer(mCameraAdapterParameters.mHandleComp,
                    (OMX_BUFFERHEADERTYPE*)portData->mBufferHeader[index]);
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
            CameraHal::PPM("startReprocess buffers queued on video port: ", &mStartCapture);
#endif

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::stopReprocess()
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters *portData = NULL;

    if (!mReprocConfigured) {
        return NO_ERROR;
    }

    portData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoInPortIndex];

    // Disable port - send command and then free all buffers
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandPortDisable,
                                mCameraAdapterParameters.mVideoInPortIndex,
                                mStopReprocSem);
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                OMX_CommandPortDisable,
                                mCameraAdapterParameters.mVideoInPortIndex,
                                NULL);
    if (portData) {
        CAMHAL_LOGDB("Freeing buffers on reproc port - num: %d", portData->mNumBufs);
        for (int index = 0 ; index < portData->mNumBufs ; index++) {
            CAMHAL_LOGDB("Freeing buffer on reproc port - 0x%x",
                         ( unsigned int ) portData->mBufferHeader[index]->pBuffer);
            eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                                    mCameraAdapterParameters.mVideoInPortIndex,
                                    (OMX_BUFFERHEADERTYPE*)portData->mBufferHeader[index]);
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }
    }
    CAMHAL_LOGDA("Waiting for port disable");
    ret = mStopReprocSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if (mComponentState == OMX_StateInvalid) {
        CAMHAL_LOGEA("Invalid State after Disable Image Port Exitting!!!");
        goto EXIT;
    }
    if (NO_ERROR == ret) {
        CAMHAL_LOGDA("Port disabled");
    } else {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mVideoInPortIndex,
                           NULL);
        CAMHAL_LOGDA("Timeout expired on port disable");
        goto EXIT;
    }

    deinitInternalBuffers(mCameraAdapterParameters.mVideoInPortIndex);

    mReprocConfigured = false;

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::disableReprocess(){
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    // no-op..for now

EXIT:
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::UseBuffersReprocess(CameraBuffer *bufArr, int num)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters *portData = NULL;

    portData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoInPortIndex];

    if ( 0 != mUseReprocessSem.Count() ) {
        CAMHAL_LOGEB("Error mUseReprocessSem semaphore count %d", mUseReprocessSem.Count());
        return BAD_VALUE;
    }

    CAMHAL_ASSERT(num > 0);

    if (mAdapterState == REPROCESS_STATE) {
        stopReprocess();
    } else if (mAdapterState == CAPTURE_STATE) {
        stopImageCapture();
        stopReprocess();
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Reprocess stopping image capture and disabling image port: ", &bufArr->ppmStamp);

#endif

    portData->mNumBufs = num;

    // Configure
    ret = setParametersReprocess(mParams, bufArr, mAdapterState);

    if (mReprocConfigured) {
        if (mPendingReprocessSettings & ECaptureParamSettings) {
            stopReprocess();
        } else {
            // Tap in port has been already configured.
            return NO_ERROR;
        }
    }

    if (mPendingReprocessSettings & SetFormat) {
        mPendingReprocessSettings &= ~SetFormat;
        ret = setFormat(OMX_CAMERA_PORT_VIDEO_IN_VIDEO, *portData);
        if ( ret != NO_ERROR ) {
            CAMHAL_LOGEB("setFormat() failed %d", ret);
            LOG_FUNCTION_NAME_EXIT;
            return ret;
        }
    }

    // Configure DOMX to use either gralloc handles or vptrs
    OMX_TI_PARAMUSENATIVEBUFFER domxUseGrallocHandles;
    OMX_INIT_STRUCT_PTR (&domxUseGrallocHandles, OMX_TI_PARAMUSENATIVEBUFFER);

    domxUseGrallocHandles.nPortIndex = mCameraAdapterParameters.mVideoInPortIndex;
    if (bufArr[0].type == CAMERA_BUFFER_ANW) {
        CAMHAL_LOGD("Using ANW");
        domxUseGrallocHandles.bEnable = OMX_TRUE;

        // Need to allocate tiler reservation and state we are going to be using
        // pagelist buffers. Assuming this happens when buffers if from anw
        initInternalBuffers(mCameraAdapterParameters.mVideoInPortIndex);
    } else {
        CAMHAL_LOGD("Using ION");
        domxUseGrallocHandles.bEnable = OMX_FALSE;
    }
    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_TI_IndexUseNativeBuffers, &domxUseGrallocHandles);
    if (eError!=OMX_ErrorNone) {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Reprocess configuration done: ", &bufArr->ppmStamp);

#endif

    // Enable Port
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mVideoInPortIndex,
                           mUseReprocessSem);
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortEnable,
                             mCameraAdapterParameters.mVideoInPortIndex,
                             NULL);
    GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

    for (int index = 0 ; index < portData->mNumBufs ; index++)
    {
        OMX_BUFFERHEADERTYPE *pBufferHdr;
        CAMHAL_LOGDB("OMX_UseBuffer Capture address: 0x%x, size = %d",
                     (unsigned int)bufArr[index].opaque,
                     (int)portData->mBufSize);

        eError = OMX_UseBuffer(mCameraAdapterParameters.mHandleComp,
                               &pBufferHdr,
                               mCameraAdapterParameters.mVideoInPortIndex,
                               0,
                               portData->mBufSize,
                               (OMX_U8*)camera_buffer_get_omx_ptr(&bufArr[index]));

        CAMHAL_LOGDB("OMX_UseBuffer = 0x%x", eError);
        GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

        pBufferHdr->pAppPrivate = (OMX_PTR) &bufArr[index];
        bufArr[index].index = index;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion.s.nVersionMajor = 1 ;
        pBufferHdr->nVersion.s.nVersionMinor = 1 ;
        pBufferHdr->nVersion.s.nRevision = 0;
        pBufferHdr->nVersion.s.nStep =  0;
        pBufferHdr->nOffset = bufArr[index].offset;
        pBufferHdr->nFilledLen = bufArr[index].actual_size;
        portData->mBufferHeader[index] = pBufferHdr;
    }

    // Wait for port enable event
    CAMHAL_LOGDA("Waiting for port enable");
    ret = mUseReprocessSem.WaitTimeout(OMX_CMD_TIMEOUT);

    // Error out if somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid) {
        CAMHAL_LOGEA("Invalid State while trying to enable port for reprocessing");
        goto EXIT;
    }

    if (ret == NO_ERROR) {
        CAMHAL_LOGDA("Port enabled");
    } else {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mVideoInPortIndex,
                           NULL);
        CAMHAL_LOGDA("Timeout expired on port enable");
        goto EXIT;
    }

    mReprocConfigured = true;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    CameraHal::PPM("Reprocess video port enabled and buffers registered: ", &bufArr->ppmStamp);

#endif

    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    // Release image buffers
    if ( NULL != mReleaseImageBuffersCallback ) {
        mReleaseImageBuffersCallback(mReleaseData);
    }
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | Utils::ErrorUtils::omxToAndroidError(eError));

}

} // namespace Camera
} // namespace Ti
