/*
 * Copyright (C) 2012 Intel Corporation.  All rights reserved.
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
 *
 */

#include <math.h>
#include <utils/Errors.h>
#include "isv_processor.h"
#include "isv_profile.h"
#include "isv_omxcomponent.h"

//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "isv-omxil"

using namespace android;

#define MAX_RETRY_NUM   10

ISVProcessor::ISVProcessor(bool canCallJava,
        sp<ISVBufferManager> bufferManager,
        sp<ISVProcessorObserver> owner,
        uint32_t width, uint32_t height)
    :Thread(canCallJava),
    mpOwner(owner),
    mThreadId(NULL),
    mThreadRunning(false),
    mISVWorker(NULL),
    mBufferManager(bufferManager),
    mOutputProcIdx(0),
    mInputProcIdx(0),
    mNumTaskInProcesing(0),
    mNumRetry(0),
    mError(false),
    mbFlush(false),
    mbBypass(false),
    mFlagEnd(false),
    mFilters(0)
{
    //FIXME: for 1920 x 1088, we also consider it as 1080p
    mISVProfile = new ISVProfile(width, (height == 1088) ? 1080 : height);

    // get platform ISV cap first
    mFilters = mISVProfile->getFilterStatus();

    // turn off filters if dynamic vpp/frc setting is off
    if (!ISVProfile::isVPPOn())
        mFilters &= FilterFrameRateConversion;

    if (!ISVProfile::isFRCOn())
        mFilters &= ~FilterFrameRateConversion;

    //FIXME: move this into profile.
    if (width > 2048)
        mFilters &= ~FilterSharpening;

    memset(&mFilterParam, 0, sizeof(mFilterParam));
    //FIXME: we don't support scaling yet, so set src region equal to dst region
    mFilterParam.srcWidth = mFilterParam.dstWidth = width;
    mFilterParam.srcHeight = mFilterParam.dstHeight = height;
    mOutputBuffers.clear();
    mInputBuffers.clear();
    mTimeWindow.clear();
}

ISVProcessor::~ISVProcessor() {
    ALOGV("ISVProcessor is deleted");
    flush();
    mOutputBuffers.clear();
    mInputBuffers.clear();

    mISVProfile = NULL;
    mFilters = 0;
    memset(&mFilterParam, 0, sizeof(mFilterParam));
}

status_t ISVProcessor::readyToRun()
{
    mThreadId = androidGetThreadId();
    //do init ops here
    return Thread::readyToRun();
}

void ISVProcessor::start()
{
    ALOGD_IF(ISV_THREAD_DEBUG, "ISVProcessor::start");

    if (mISVWorker == NULL) {
        mISVWorker = new ISVWorker();
        if (STATUS_OK != mISVWorker->init(mFilterParam.srcWidth, mFilterParam.srcHeight))
            ALOGE("%s: mISVWorker init failed", __func__);
    }

    mBufferManager->setWorker(mISVWorker);

    this->run("ISVProcessor", ANDROID_PRIORITY_NORMAL);
    mThreadRunning = true;
    return;
}

void ISVProcessor::stop()
{
    ALOGD_IF(ISV_THREAD_DEBUG, "ISVProcessor::stop");

    if(mThreadRunning) {
        this->requestExit();
        {
            Mutex::Autolock autoLock(mLock);
            mRunCond.signal();
        }
        this->requestExitAndWait();
        mThreadRunning = false;
    }

    if (STATUS_OK != mISVWorker->deinit())
        ALOGE("%s: mISVWorker deinit failed", __func__);

    mISVWorker = NULL;
    return;
}

bool ISVProcessor::getBufForFirmwareOutput(Vector<ISVBuffer*> *fillBufList,uint32_t *fillBufNum){
    uint32_t i = 0;
    // output buffer number for filling
    *fillBufNum = 0;
    uint32_t needFillNum = 0;
    OMX_BUFFERHEADERTYPE *outputBuffer;

    //output data available
    needFillNum = mISVWorker->getFillBufCount();
    if (mOutputProcIdx < needFillNum ||
            mInputProcIdx < 1) {
        ALOGE("%s: no enough input or output buffer which need to be sync", __func__);
        return false;
    }

    if ((needFillNum == 0) || (needFillNum > 4))
       return false;

    Mutex::Autolock autoLock(mOutputLock);
    for (i = 0; i < needFillNum; i++) {
        //fetch the render buffer from the top of output buffer queue
        outputBuffer = mOutputBuffers.itemAt(i);
        if (!outputBuffer) {
            ALOGE("%s: failed to fetch output buffer for sync.", __func__);
            return false;
        }
        unsigned long fillHandle = reinterpret_cast<unsigned long>(outputBuffer->pBuffer);
        ISVBuffer* fillBuf = mBufferManager->mapBuffer(fillHandle);
        fillBufList->push_back(fillBuf);
    }

    *fillBufNum  = i;
    return true;
}


status_t ISVProcessor::updateFirmwareOutputBufStatus(uint32_t fillBufNum) {
    int64_t timeUs;
    OMX_BUFFERHEADERTYPE *outputBuffer;
    OMX_BUFFERHEADERTYPE *inputBuffer;
    OMX_ERRORTYPE err;
    bool cropChanged = false;

    if (mInputBuffers.empty()) {
        ALOGE("%s: input buffer queue is empty. no buffer need to be sync", __func__);
        return UNKNOWN_ERROR;
    }

    if (mOutputBuffers.size() < fillBufNum) {
        ALOGE("%s: no enough output buffer which need to be sync", __func__);
        return UNKNOWN_ERROR;
    }
    // remove one buffer from intput buffer queue
    {
        Mutex::Autolock autoLock(mInputLock);
        inputBuffer = mInputBuffers.itemAt(0);
        unsigned long inputHandle = reinterpret_cast<unsigned long>(inputBuffer->pBuffer);
        ISVBuffer* inputBuf = mBufferManager->mapBuffer(inputHandle);
        uint32_t flags = inputBuf->getFlags();

        if (flags & ISVBuffer::ISV_BUFFER_CROP_CHANGED) {
            err = mpOwner->reportOutputCrop();
            if (err != OMX_ErrorNone) {
                ALOGE("%s: failed to reportOutputCrop", __func__);
                return UNKNOWN_ERROR;
            }
            cropChanged = true;
            inputBuf->unsetFlag(ISVBuffer::ISV_BUFFER_CROP_CHANGED);
        }

        err = mpOwner->releaseBuffer(kPortIndexInput, inputBuffer, false);
        if (err != OMX_ErrorNone) {
            ALOGE("%s: failed to fillInputBuffer", __func__);
            return UNKNOWN_ERROR;
        }

        mInputBuffers.removeAt(0);
        ALOGD_IF(ISV_THREAD_DEBUG, "%s: fetch buffer %u from input buffer queue for fill to decoder, and then queue size is %d", __func__,
                inputBuffer, mInputBuffers.size());
        mInputProcIdx--;
    }

    //set the time stamp for interpreted frames
    {
        Mutex::Autolock autoLock(mOutputLock);
        timeUs = mOutputBuffers[0]->nTimeStamp;

        for(uint32_t i = 0; i < fillBufNum; i++) {
            outputBuffer = mOutputBuffers.itemAt(i);
            if (fillBufNum > 1) {
                if (mFilterParam.frameRate == 24) {
                    if (fillBufNum == 2) {
                        outputBuffer->nTimeStamp = timeUs + 1000000ll * (i + 1) / 60 - 1000000ll * 1 / 24;
                    } else if (fillBufNum == 3) {
                        outputBuffer->nTimeStamp = timeUs + 1000000ll * (i + 3) / 60 - 1000000ll * 2 / 24;
                    }
                }
                else
                    outputBuffer->nTimeStamp = timeUs - 1000000ll * (fillBufNum - i - 1) / (mFilterParam.frameRate * 2);
            }

            //return filled buffers for rendering
            //skip rendering for crop change
            err = mpOwner->releaseBuffer(kPortIndexOutput, outputBuffer, cropChanged);

            if (err != OMX_ErrorNone) {
                ALOGE("%s: failed to releaseOutputBuffer", __func__);
                return UNKNOWN_ERROR;
            }

            ALOGD_IF(ISV_THREAD_DEBUG, "%s: fetch buffer %u(timestamp %.2f ms) from output buffer queue for render, and then queue size is %d", __func__,
                    outputBuffer, outputBuffer->nTimeStamp/1E3, mOutputBuffers.size());
        }
        // remove filled buffers from output buffer queue
        mOutputBuffers.removeItemsAt(0, fillBufNum);
        mOutputProcIdx -= fillBufNum;
    }
    return OK;
}


bool ISVProcessor::getBufForFirmwareInput(Vector<ISVBuffer*> *procBufList,
                                   ISVBuffer **inputBuf,
                                   uint32_t *procBufNum)
{
    OMX_BUFFERHEADERTYPE *outputBuffer;
    OMX_BUFFERHEADERTYPE *inputBuffer;

    if (mbFlush) {
        *inputBuf = NULL;
        *procBufNum = 0;
        return true;
    }

    int32_t procBufCount = mISVWorker->getProcBufCount();
    if ((procBufCount == 0) || (procBufCount > 4)) {
       return false;
    }

    //fetch a input buffer for processing
    {
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: acqiring mInputLock", __func__);
        Mutex::Autolock autoLock(mInputLock);
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: acqired mInputLock", __func__);
        inputBuffer = mInputBuffers.itemAt(mInputProcIdx);
        if (!inputBuffer) {
            ALOGE("%s: failed to get input buffer for processing.", __func__);
            return false;
        }
        unsigned long inputHandle = reinterpret_cast<unsigned long>(inputBuffer->pBuffer);
        *inputBuf = mBufferManager->mapBuffer(inputHandle);
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: releasing mInputLock", __func__);
    }

    //fetch output buffers for processing
    {
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: acqiring mOutputLock", __func__);
        Mutex::Autolock autoLock(mOutputLock);
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: acqired mOutputLock", __func__);
        for (int32_t i = 0; i < procBufCount; i++) {
            outputBuffer = mOutputBuffers.itemAt(mOutputProcIdx + i);
            if (!outputBuffer) {
                ALOGE("%s: failed to get output buffer for processing.", __func__);
                return false;
            }
            unsigned long outputHandle = reinterpret_cast<unsigned long>(outputBuffer->pBuffer);
            procBufList->push_back(mBufferManager->mapBuffer(outputHandle));
        }
        *procBufNum = procBufCount;
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: releasing mOutputLock", __func__);
    }

    return true;
}


status_t ISVProcessor::updateFirmwareInputBufStatus(uint32_t procBufNum)
{
    OMX_BUFFERHEADERTYPE *outputBuffer;
    OMX_BUFFERHEADERTYPE *inputBuffer;

    inputBuffer = mInputBuffers.itemAt(mInputProcIdx);
    mInputProcIdx++;

    Mutex::Autolock autoLock(mOutputLock);
    for(uint32_t i = 0; i < procBufNum; i++) {
        outputBuffer = mOutputBuffers.editItemAt(mOutputProcIdx + i);
        // set output buffer timestamp as the same as input
        outputBuffer->nTimeStamp = inputBuffer->nTimeStamp;
        outputBuffer->nFilledLen = inputBuffer->nFilledLen;
        outputBuffer->nOffset = inputBuffer->nOffset;
        outputBuffer->nFlags = inputBuffer->nFlags;
        //outputBuffer->nTickCount = inputBuffer->nTickCount;
        //outputBuffer->pMarkData = intputBuffer->pMarkData;
    }
    mOutputProcIdx += procBufNum;
    return OK;
}


bool ISVProcessor::isReadytoRun()
{
    ALOGD_IF(ISV_THREAD_DEBUG, "%s: mISVWorker->getProcBufCount() return %d", __func__,
            mISVWorker->getProcBufCount());
    if (mInputProcIdx < mInputBuffers.size() 
            && (mOutputBuffers.size() - mOutputProcIdx) >= mISVWorker->getProcBufCount())
       return true;
    else
       return false;
}


bool ISVProcessor::threadLoop() {
    uint32_t procBufNum = 0, fillBufNum = 0;
    ISVBuffer* inputBuf;
    Vector<ISVBuffer*> procBufList;
    Vector<ISVBuffer*> fillBufList;
    uint32_t flags = 0;
    bool bGetBufSuccess = true;

    Mutex::Autolock autoLock(mLock);

    if (!isReadytoRun() && !mbFlush) {
        mRunCond.wait(mLock);
    }

    if (isReadytoRun() || mbFlush) {
        procBufList.clear();
        bool bGetInBuf = getBufForFirmwareInput(&procBufList, &inputBuf, &procBufNum);
        if (bGetInBuf) {
            if (!mbFlush)
                flags = mInputBuffers[mInputProcIdx]->nFlags;
            status_t ret = mISVWorker->process(inputBuf, procBufList, procBufNum, mbFlush, flags);
            if (ret == STATUS_OK) {
                // for seek and EOS
                if (mbFlush) {
                    mISVWorker->reset();
                    flush();

                    mNumTaskInProcesing = 0;
                    mInputProcIdx = 0;
                    mOutputProcIdx = 0;

                    mbFlush = false;

                    Mutex::Autolock endLock(mEndLock);
                    mEndCond.signal();
                    return true;
                }
                mNumTaskInProcesing++;
                updateFirmwareInputBufStatus(procBufNum);
            } else {
                mbBypass = true;
                flush();
                ALOGE("VSP process error %d .... ISV changes to bypass mode", __LINE__);
            }
        }
    }

    ALOGV("mNumTaskInProcesing %d", mNumTaskInProcesing);
    while ((mNumTaskInProcesing > 0) && mNumTaskInProcesing >= mISVWorker->mNumForwardReferences && bGetBufSuccess ) {
        fillBufList.clear();
        bGetBufSuccess = getBufForFirmwareOutput(&fillBufList, &fillBufNum);
        ALOGD_IF(ISV_THREAD_DEBUG, "%s: bGetOutput %d, buf num %d", __func__,
                bGetBufSuccess, fillBufNum);
        if (bGetBufSuccess) {
            status_t ret = mISVWorker->fill(fillBufList, fillBufNum);
            if (ret == STATUS_OK) {
                mNumTaskInProcesing--;
                ALOGV("mNumTaskInProcesing: %d ...", mNumTaskInProcesing);
                updateFirmwareOutputBufStatus(fillBufNum);
            } else {
                mError = true;
                ALOGE("ISV read firmware data error! Thread EXIT...");
                return false;
            }
        }
    }

    return true;
}

bool ISVProcessor::isCurrentThread() const {
    return mThreadId == androidGetThreadId();
}

inline bool ISVProcessor::isFrameRateValid(uint32_t fps)
{
    return (fps == 15 || fps == 24 || fps == 25 || fps == 30 || fps == 50 || fps == 60) ? true : false;
}

status_t ISVProcessor::configFRC(uint32_t fps)
{
    if (isFrameRateValid(fps)) {
        if (fps == 50 || fps == 60) {
            ALOGD_IF(ISV_THREAD_DEBUG, "%s: %d fps don't need do FRC, so disable FRC", __func__, fps);
            mFilters &= ~FilterFrameRateConversion;
            mFilterParam.frcRate = FRC_RATE_1X;
        } else {
            mFilterParam.frameRate = fps;
            mFilterParam.frcRate = mISVProfile->getFRCRate(mFilterParam.frameRate);
            ALOGD_IF(ISV_THREAD_DEBUG, "%s: fps is set to %d, frc rate is %d", __func__,
                    mFilterParam.frameRate, mFilterParam.frcRate);
        }
        return OK;
    }

    return UNKNOWN_ERROR;
}

status_t ISVProcessor::calculateFps(int64_t timeStamp, uint32_t* fps)
{
    int32_t i = 0;
    *fps = 0;

    mTimeWindow.push_back(timeStamp);
    if (mTimeWindow.size() > WINDOW_SIZE) {
        mTimeWindow.removeAt(0);
    }
    else if (mTimeWindow.size() < WINDOW_SIZE)
        return NOT_ENOUGH_DATA;

    int64_t delta = mTimeWindow[WINDOW_SIZE-1] - mTimeWindow[0];
    if (delta == 0)
        return NOT_ENOUGH_DATA;

    *fps = ceil(1.0 / delta * 1E6 * (WINDOW_SIZE-1));
    return OK;
}

status_t ISVProcessor::configFilters(OMX_BUFFERHEADERTYPE* buffer)
{
    if ((mFilters & FilterFrameRateConversion) != 0) {
        if (!isFrameRateValid(mFilterParam.frameRate)) {
            if (mNumRetry++ < MAX_RETRY_NUM) {
                uint32_t fps = 0;
                if (OK != calculateFps(buffer->nTimeStamp, &fps))
                    return NOT_ENOUGH_DATA;

                if (OK != configFRC(fps))
                    return NOT_ENOUGH_DATA;
            } else {
                ALOGD_IF(ISV_THREAD_DEBUG, "%s: exceed max retry to get a valid frame rate(%d), disable FRC", __func__,
                        mFilterParam.frameRate);
                mFilters &= ~FilterFrameRateConversion;
                mFilterParam.frcRate = FRC_RATE_1X;
            }
        }
    }

    if ((buffer->nFlags & OMX_BUFFERFLAG_TFF) != 0 ||
            (buffer->nFlags & OMX_BUFFERFLAG_BFF) != 0)
        mFilters |= FilterDeinterlacing;
    else
        mFilters &= ~FilterDeinterlacing;

    if (mFilters == 0) {
        ALOGI("%s: no filter need to be config, bypass ISV", __func__);
        return UNKNOWN_ERROR;
    }

    //config filters to mISVWorker
    return (mISVWorker->configFilters(mFilters, &mFilterParam) == STATUS_OK) ? OK : UNKNOWN_ERROR;
}

void ISVProcessor::addInput(OMX_BUFFERHEADERTYPE* input)
{
    if (mbFlush) {
        mpOwner->releaseBuffer(kPortIndexInput, input, true);
        return;
    }

    if (mbBypass) {
        // return this buffer to framework
        mpOwner->releaseBuffer(kPortIndexOutput, input, false);
        return;
    }

    if (input->nFlags & OMX_BUFFERFLAG_EOS) {
        mpOwner->releaseBuffer(kPortIndexInput, input, true);
        notifyFlush();
        return;
    }

    status_t ret = configFilters(input);
    if (ret == NOT_ENOUGH_DATA) {
        // release this buffer if frc is not ready.
        mpOwner->releaseBuffer(kPortIndexInput, input, false);
        ALOGD_IF(ISV_THREAD_DEBUG, "%s: frc rate is not ready, release this buffer %u, fps %d", __func__,
                input, mFilterParam.frameRate);
        return;
    } else if (ret == UNKNOWN_ERROR) {
        ALOGD_IF(ISV_THREAD_DEBUG, "%s: configFilters failed, bypass ISV", __func__);
        mbBypass = true;
        mpOwner->releaseBuffer(kPortIndexOutput, input, false);
        return;
    }

    {
        //put the decoded buffer into fill buffer queue
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: acqiring mInputLock", __func__);
        Mutex::Autolock autoLock(mInputLock);
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: acqired mInputLock", __func__);

        mInputBuffers.push_back(input);
        ALOGD_IF(ISV_THREAD_DEBUG, "%s: hold pBuffer %u in input buffer queue. Intput queue size is %d, mInputProIdx %d.\
                Output queue size is %d, mOutputProcIdx %d", __func__,
                input, mInputBuffers.size(), mInputProcIdx,
                mOutputBuffers.size(), mOutputProcIdx);
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: releasing mInputLock", __func__);
    }

    {
        Mutex::Autolock autoLock(mLock);
        mRunCond.signal();
    }
    return;
}

void ISVProcessor::addOutput(OMX_BUFFERHEADERTYPE* output)
{
    if (mbFlush) {
        mpOwner->releaseBuffer(kPortIndexOutput, output, true);
        return;
    }

    if (mbBypass || mOutputBuffers.size() >= MIN_OUTPUT_NUM) {
        // return this buffer to decoder
        mpOwner->releaseBuffer(kPortIndexInput, output, false);
        return;
    }

    {
        //push the buffer into the output queue if it is not full
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: acqiring mOutputLock", __func__);
        Mutex::Autolock autoLock(mOutputLock);
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: acqired mOutputLock", __func__);

        mOutputBuffers.push_back(output);
        ALOGD_IF(ISV_THREAD_DEBUG, "%s: hold pBuffer %u in output buffer queue. Input queue size is %d, mInputProIdx %d.\
                Output queue size is %d, mOutputProcIdx %d", __func__,
                output, mInputBuffers.size(), mInputProcIdx,
                mOutputBuffers.size(), mOutputProcIdx);
        ALOGD_IF(ISV_COMPONENT_LOCK_DEBUG, "%s: releasing mOutputLock", __func__);
    }

    {
        Mutex::Autolock autoLock(mLock);
        mRunCond.signal();
    }
    return;
}

void ISVProcessor::notifyFlush()
{
    if (mInputBuffers.empty() && mOutputBuffers.empty()) {
        ALOGD_IF(ISV_THREAD_DEBUG, "%s: input and ouput buffer queue is empty, nothing need to do", __func__);
        return;
    }

    Mutex::Autolock autoLock(mLock);
    mbFlush = true;
    mRunCond.signal();
    ALOGD_IF(ISV_THREAD_DEBUG, "wake up proc thread");
    return;
}

void ISVProcessor::waitFlushFinished()
{
    Mutex::Autolock endLock(mEndLock);
    ALOGD_IF(ISV_THREAD_DEBUG, "waiting mEnd lock(seek finish) ");
    while(mbFlush) {
        mEndCond.wait(mEndLock);
    }
    return;
}

void ISVProcessor::flush()
{
    OMX_BUFFERHEADERTYPE* pBuffer = NULL;
    {
        Mutex::Autolock autoLock(mInputLock);
        while (!mInputBuffers.empty()) {
            pBuffer = mInputBuffers.itemAt(0);
            mpOwner->releaseBuffer(kPortIndexInput, pBuffer, true);
            ALOGD_IF(ISV_THREAD_DEBUG, "%s: Flush the pBuffer %u in input buffer queue.", __func__, pBuffer);
            mInputBuffers.removeAt(0);
        }
    }
    {
        Mutex::Autolock autoLock(mOutputLock);
        while (!mOutputBuffers.empty()) {
            pBuffer = mOutputBuffers.itemAt(0);
            mpOwner->releaseBuffer(kPortIndexOutput, pBuffer, true);
            ALOGD_IF(ISV_THREAD_DEBUG, "%s: Flush the pBuffer %u in output buffer queue.", __func__, pBuffer);
            mOutputBuffers.removeAt(0);
        }
    }
    //flush finished.
    return;
}
