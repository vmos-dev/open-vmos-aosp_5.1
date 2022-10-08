/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "PVSoftMPEG4Encoder"
#include <wrs_omxil_core/log.h>

#include "mp4enc_api.h"
#include "OMX_Video.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>

#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>

#include "PVSoftMPEG4Encoder.h"
#include "VideoEncoderLog.h"

#define ALIGN(x, align)                  (((x) + (align) - 1) & (~((align) - 1)))

inline static void ConvertYUV420SemiPlanarToYUV420Planar(
        uint8_t *inyuv, uint8_t* outyuv,
        int32_t width, int32_t height) {

    int32_t outYsize = width * height;
    uint32_t *outy =  (uint32_t *) outyuv;
    uint16_t *outcb = (uint16_t *) (outyuv + outYsize);
    uint16_t *outcr = (uint16_t *) (outyuv + outYsize + (outYsize >> 2));

    /* Y copying */
    memcpy(outy, inyuv, outYsize);

    /* U & V copying */
    uint32_t *inyuv_4 = (uint32_t *) (inyuv + outYsize);
    for (int32_t i = height >> 1; i > 0; --i) {
        for (int32_t j = width >> 2; j > 0; --j) {
            uint32_t temp = *inyuv_4++;
            uint32_t tempU = temp & 0xFF;
            tempU = tempU | ((temp >> 8) & 0xFF00);

            uint32_t tempV = (temp >> 8) & 0xFF;
            tempV = tempV | ((temp >> 16) & 0xFF00);

            // Flip U and V
            *outcb++ = tempU;
            *outcr++ = tempV;
        }
    }
}

inline static void trimBuffer(uint8_t *dataIn, uint8_t *dataOut,
        int32_t width, int32_t height,
        int32_t alignedHeight, int32_t stride) {
    int32_t h;
    uint8_t *y_start, *uv_start, *_y_start, *_uv_start;
    y_start = dataOut;
    uv_start = dataOut + width * height;
    _y_start = dataIn;
    _uv_start = dataIn + stride * alignedHeight;

    for (h = 0; h < height; h++)
        memcpy(y_start + h * width, _y_start + h * stride, width);
    for (h = 0; h < height / 2; h++)
        memcpy(uv_start + h * width,
                _uv_start + h * stride, width);
}

PVSoftMPEG4Encoder::PVSoftMPEG4Encoder(const char *name)
    :  mEncodeMode(COMBINE_MODE_WITH_ERR_RES),
      mVideoWidth(176),
      mVideoHeight(144),
      mVideoFrameRate(30),
      mVideoBitRate(192000),
      mVideoColorFormat(OMX_COLOR_FormatYUV420SemiPlanar),
      mStoreMetaDataInBuffers(false),
      mIDRFrameRefreshIntervalInSec(1),
      mNumInputFrames(-1),
      mStarted(false),
      mSawInputEOS(false),
      mSignalledError(false),
      mHandle(new tagvideoEncControls),
      mEncParams(new tagvideoEncOptions),
      mInputFrameData(NULL)
{

    if (!strcmp(name, "OMX.google.h263.encoder")) {
        mEncodeMode = H263_MODE;
        LOG_I("construct h263 encoder");
    } else {
        CHECK(!strcmp(name, "OMX.google.mpeg4.encoder"));
        LOG_I("construct mpeg4 encoder");
    }

    setDefaultParams();
#if NO_BUFFER_SHARE
    mVASurfaceMappingAction |= MAPACT_COPY;
#endif

    LOG_I("Construct PVSoftMPEG4Encoder");

}

PVSoftMPEG4Encoder::~PVSoftMPEG4Encoder() {
    LOG_I("Destruct PVSoftMPEG4Encoder");
    releaseEncoder();

}

void PVSoftMPEG4Encoder::setDefaultParams() {

    // Set default value for input parameters
    mComParams.profile = VAProfileH264Baseline;
    mComParams.level = 41;
    mComParams.rawFormat = RAW_FORMAT_NV12;
    mComParams.frameRate.frameRateNum = 30;
    mComParams.frameRate.frameRateDenom = 1;
    mComParams.resolution.width = 0;
    mComParams.resolution.height = 0;
    mComParams.intraPeriod = 30;
    mComParams.rcMode = RATE_CONTROL_NONE;
    mComParams.rcParams.initQP = 15;
    mComParams.rcParams.minQP = 0;
    mComParams.rcParams.bitRate = 640000;
    mComParams.rcParams.targetPercentage= 0;
    mComParams.rcParams.windowSize = 0;
    mComParams.rcParams.disableFrameSkip = 0;
    mComParams.rcParams.disableBitsStuffing = 1;
    mComParams.cyclicFrameInterval = 30;
    mComParams.refreshType = VIDEO_ENC_NONIR;
    mComParams.airParams.airMBs = 0;
    mComParams.airParams.airThreshold = 0;
    mComParams.airParams.airAuto = 1;
    mComParams.disableDeblocking = 2;
    mComParams.syncEncMode = false;
    mComParams.codedBufNum = 2;

}

Encode_Status PVSoftMPEG4Encoder::initEncParams() {
    CHECK(mHandle != NULL);
    memset(mHandle, 0, sizeof(tagvideoEncControls));

    CHECK(mEncParams != NULL);
    memset(mEncParams, 0, sizeof(tagvideoEncOptions));
    if (!PVGetDefaultEncOption(mEncParams, 0)) {
        LOG_E("Failed to get default encoding parameters");
        return ENCODE_FAIL;
    }
    mEncParams->encMode = mEncodeMode;
    mEncParams->encWidth[0] = mVideoWidth;
    mEncParams->encHeight[0] = mVideoHeight;
    mEncParams->encFrameRate[0] = mVideoFrameRate;
    mEncParams->rcType = VBR_1;
    mEncParams->vbvDelay = 5.0f;

    // FIXME:
    // Add more profile and level support for MPEG4 encoder
    mEncParams->profile_level = CORE_PROFILE_LEVEL2;
    mEncParams->packetSize = 32;
    mEncParams->rvlcEnable = PV_OFF;
    mEncParams->numLayers = 1;
    mEncParams->timeIncRes = 1000;
    mEncParams->tickPerSrc = mEncParams->timeIncRes / mVideoFrameRate;

    mEncParams->bitRate[0] = mVideoBitRate <= 2000000 ? mVideoBitRate : 2000000;
    mEncParams->iQuant[0] = 15;
    mEncParams->pQuant[0] = 12;
    mEncParams->quantType[0] = 0;
    mEncParams->noFrameSkipped = PV_OFF;

    mTrimedInputData =
        (uint8_t *) malloc((mVideoWidth * mVideoHeight * 3 ) >> 1);
    CHECK(mTrimedInputData != NULL);

    if (mVideoColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
        // Color conversion is needed.
        CHECK(mInputFrameData == NULL);
        mInputFrameData =
            (uint8_t *) malloc((mVideoWidth * mVideoHeight * 3 ) >> 1);
        CHECK(mInputFrameData != NULL);
    }

    // PV's MPEG4 encoder requires the video dimension of multiple
    if (mVideoWidth % 16 != 0 || mVideoHeight % 16 != 0) {
        LOG_E("Video frame size %dx%d must be a multiple of 16",
            mVideoWidth, mVideoHeight);
        return ENCODE_INVALID_PARAMS;
    }

    // Set IDR frame refresh interval
    if (mIDRFrameRefreshIntervalInSec < 0) {
        mEncParams->intraPeriod = -1;
    } else if (mIDRFrameRefreshIntervalInSec == 0) {
        mEncParams->intraPeriod = 1;  // All I frames
    } else {
        mEncParams->intraPeriod =
            (mIDRFrameRefreshIntervalInSec * mVideoFrameRate);
    }

    mEncParams->numIntraMB = 0;
    mEncParams->sceneDetect = PV_ON;
    mEncParams->searchRange = 16;
    mEncParams->mv8x8Enable = PV_OFF;
    mEncParams->gobHeaderInterval = 0;
    mEncParams->useACPred = PV_ON;
    mEncParams->intraDCVlcTh = 0;

    return ENCODE_SUCCESS;
}

Encode_Status PVSoftMPEG4Encoder::initEncoder() {
    LOG_V("Begin\n");

    CHECK(!mStarted);

    Encode_Status ret = ENCODE_SUCCESS;
    if (ENCODE_SUCCESS != (ret = initEncParams())) {
        LOG_E("Failed to initialized encoder params");
        mSignalledError = true;
        return ret;
    }

    if (!PVInitVideoEncoder(mHandle, mEncParams)) {
        LOG_E("Failed to initialize the encoder");
        mSignalledError = true;
        return ENCODE_FAIL;
    }

    mNumInputFrames = -1;  // 1st buffer for codec specific data
    mStarted = true;
    mCurTimestampUs = 0;
    mLastTimestampUs = 0;
    mVolHeaderLength = 256;

    LOG_V("End\n");

    return ENCODE_SUCCESS;
}

Encode_Status PVSoftMPEG4Encoder::releaseEncoder() {
    LOG_V("Begin\n");

    if (!mStarted) {
        return ENCODE_SUCCESS;
    }

    PVCleanUpVideoEncoder(mHandle);

    delete mTrimedInputData;
    mTrimedInputData = NULL;

    delete mInputFrameData;
    mInputFrameData = NULL;

    delete mEncParams;
    mEncParams = NULL;

    delete mHandle;
    mHandle = NULL;

    mStarted = false;

    LOG_V("End\n");

    return ENCODE_SUCCESS;
}

Encode_Status PVSoftMPEG4Encoder::setParameters(
        VideoParamConfigSet *videoEncParams)
{

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(videoEncParams);
    LOG_I("Config type = %d\n", (int)videoEncParams->type);

    if (mStarted) {
        LOG_E("Encoder has been initialized, should use setConfig to change configurations\n");
        return ENCODE_ALREADY_INIT;
    }

    switch (videoEncParams->type) {
        case VideoParamsTypeCommon: {

            VideoParamsCommon *paramsCommon =
                    reinterpret_cast <VideoParamsCommon *> (videoEncParams);
            if (paramsCommon->size != sizeof (VideoParamsCommon)) {
                return ENCODE_INVALID_PARAMS;
            }
            if(paramsCommon->codedBufNum < 2)
                paramsCommon->codedBufNum =2;
            mComParams = *paramsCommon;

            mVideoWidth = mComParams.resolution.width;
            mVideoHeight = mComParams.resolution.height;
            mVideoFrameRate =  mComParams.frameRate.frameRateNum / \
                               mComParams.frameRate.frameRateDenom;
            mVideoBitRate = mComParams.rcParams.bitRate;
            mVideoColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            break;
        }

        case VideoParamsTypeStoreMetaDataInBuffers: {
            VideoParamsStoreMetaDataInBuffers *metadata =
                    reinterpret_cast <VideoParamsStoreMetaDataInBuffers *> (videoEncParams);

            if (metadata->size != sizeof (VideoParamsStoreMetaDataInBuffers)) {
                return ENCODE_INVALID_PARAMS;
            }

            mStoreMetaDataInBuffers = metadata->isEnabled;

            break;
        }

        default: {
            LOG_I ("Wrong ParamType here\n");
            break;
        }
    }

    return ret;
}

Encode_Status PVSoftMPEG4Encoder::getParameters(
        VideoParamConfigSet *videoEncParams) {

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(videoEncParams);
    LOG_I("Config type = %d\n", (int)videoEncParams->type);

    switch (videoEncParams->type) {
        case VideoParamsTypeCommon: {

            VideoParamsCommon *paramsCommon =
                    reinterpret_cast <VideoParamsCommon *> (videoEncParams);

            if (paramsCommon->size != sizeof (VideoParamsCommon)) {
                return ENCODE_INVALID_PARAMS;
            }
            *paramsCommon = mComParams;
            break;
        }

        case VideoParamsTypeStoreMetaDataInBuffers: {
            VideoParamsStoreMetaDataInBuffers *metadata =
                    reinterpret_cast <VideoParamsStoreMetaDataInBuffers *> (videoEncParams);

            if (metadata->size != sizeof (VideoParamsStoreMetaDataInBuffers)) {
                return ENCODE_INVALID_PARAMS;
            }

            metadata->isEnabled = mStoreMetaDataInBuffers;

            break;
        }

        default: {
            LOG_I ("Wrong ParamType here\n");
            break;
        }

    }
    return ret;
}

Encode_Status PVSoftMPEG4Encoder::encode(VideoEncRawBuffer *inBuffer, uint32_t timeout)
{
    LOG_V("Begin\n");

    Encode_Status ret = ENCODE_SUCCESS;

    if (mCurTimestampUs <= inBuffer->timeStamp) {
        mLastTimestampUs = mCurTimestampUs;
        mCurTimestampUs = inBuffer->timeStamp;
    }

    if (mNumInputFrames < 0) {
        if (!PVGetVolHeader(mHandle, mVolHeader, &mVolHeaderLength, 0)) {
            LOG_E("Failed to get VOL header");
            mSignalledError = true;
            return ENCODE_FAIL;
        }
        LOG_I("Output VOL header: %d bytes", mVolHeaderLength);
        mNumInputFrames++;
        //return ENCODE_SUCCESS;
    }

    if (mStoreMetaDataInBuffers) {
        IntelMetadataBuffer imb;
        int32_t type;
        int32_t value;
        uint8_t *img;
        const android::Rect rect(mVideoWidth, mVideoHeight);
        android::status_t res;
        ValueInfo vinfo;
        ValueInfo *pvinfo = &vinfo;
        CHECK(IMB_SUCCESS == imb.UnSerialize(inBuffer->data, inBuffer->size));
        imb.GetType((::IntelMetadataBufferType&)type);
        imb.GetValue(value);
        imb.GetValueInfo(pvinfo);
        if(pvinfo == NULL) {
            res = android::GraphicBufferMapper::get().lock((buffer_handle_t)value,
                    GRALLOC_USAGE_SW_READ_MASK,
                    rect, (void**)&img);
        } else {
            img = (uint8_t*)value;
        }
        if (pvinfo != NULL)
            trimBuffer(img, mTrimedInputData, pvinfo->width, pvinfo->height,
                   pvinfo->height, pvinfo->lumaStride);
        else {
            //NV12 Y-TILED
            trimBuffer(img, mTrimedInputData, mVideoWidth, mVideoHeight,
                    ALIGN(mVideoHeight, 32), ALIGN(mVideoWidth, 128));
            android::GraphicBufferMapper::get().unlock((buffer_handle_t)value);
        }
    } else {
        memcpy(mTrimedInputData, inBuffer->data,
                (mVideoWidth * mVideoHeight * 3 ) >> 1);
    }

    if (mVideoColorFormat != OMX_COLOR_FormatYUV420Planar) {
        ConvertYUV420SemiPlanarToYUV420Planar(
                mTrimedInputData, mInputFrameData, mVideoWidth, mVideoHeight);
    } else {
        memcpy(mTrimedInputData, mInputFrameData,
                (mVideoWidth * mVideoHeight * 3 ) >> 1);
    }

    LOG_V("End\n");

    return ret;
}

Encode_Status PVSoftMPEG4Encoder::getOutput(VideoEncOutputBuffer *outBuffer, uint32_t timeout)
{
    LOG_V("Begin\n");

    Encode_Status ret = ENCODE_SUCCESS;
    uint8_t *outPtr = outBuffer->data;
    int32_t dataLength = outBuffer->bufferSize;
    outBuffer->flag = 0;

    if ((mEncodeMode == COMBINE_MODE_WITH_ERR_RES) &&
            (outBuffer->format == OUTPUT_CODEC_DATA)) {
        memcpy(outPtr, mVolHeader, mVolHeaderLength);
        ++mNumInputFrames;
        outBuffer->flag |= ENCODE_BUFFERFLAG_CODECCONFIG;
        outBuffer->flag |= ENCODE_BUFFERFLAG_ENDOFFRAME;
        outBuffer->flag |= ENCODE_BUFFERFLAG_SYNCFRAME;
        outBuffer->dataSize = mVolHeaderLength;
        outBuffer->remainingSize = 0;
        return ENCODE_SUCCESS;
    }

    outBuffer->timeStamp = mCurTimestampUs;
    LOG_I("info.mTimeUs %lld\n", outBuffer->timeStamp);

    VideoEncFrameIO vin, vout;
    memset(&vin, 0, sizeof(vin));
    memset(&vout, 0, sizeof(vout));
    vin.height = ((mVideoHeight  + 15) >> 4) << 4;
    vin.pitch = ((mVideoWidth + 15) >> 4) << 4;
    vin.timestamp = (outBuffer->timeStamp + 500) / 1000;  // in ms
    vin.yChan = mInputFrameData;
    vin.uChan = vin.yChan + vin.height * vin.pitch;
    vin.vChan = vin.uChan + ((vin.height * vin.pitch) >> 2);

    unsigned long modTimeMs = 0;
    int32_t nLayer = 0;
    MP4HintTrack hintTrack;
    if (!PVEncodeVideoFrame(mHandle, &vin, &vout,
                &modTimeMs, outPtr, &dataLength, &nLayer) ||
            !PVGetHintTrack(mHandle, &hintTrack)) {
        LOG_E("Failed to encode frame or get hink track at frame %lld",
                mNumInputFrames);
        mSignalledError = true;
        hintTrack.CodeType = 0;
        ret = ENCODE_FAIL;
    }
    LOG_I("dataLength %d\n", dataLength);
    CHECK(NULL == PVGetOverrunBuffer(mHandle));
    if (hintTrack.CodeType == 0) {  // I-frame serves as sync frame
        outBuffer->flag |= ENCODE_BUFFERFLAG_SYNCFRAME;
    }

    ++mNumInputFrames;

    outBuffer->flag |= ENCODE_BUFFERFLAG_ENDOFFRAME;
    outBuffer->dataSize = dataLength;

    LOG_V("End\n");

    return ret;
}

