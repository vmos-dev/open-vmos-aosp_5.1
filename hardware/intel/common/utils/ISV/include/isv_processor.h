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

#ifndef __ISV_PROCESSOR_H
#define __ISV_PROCESSOR_H

#include <media/stagefright/MetaData.h>
#include "isv_worker.h"

#include <utils/Mutex.h>
#include <utils/threads.h>
#include <utils/Errors.h>
#include "isv_bufmanager.h"
#define ISV_COMPONENT_LOCK_DEBUG 0
#define ISV_THREAD_DEBUG 0

using namespace android;

typedef enum {
    kPortIndexInput  = 0,
    kPortIndexOutput = 1
}PORT_INDEX;

class ISVBufferManager;
class ISVProcessorObserver: public RefBase
{
public:
    virtual OMX_ERRORTYPE releaseBuffer(PORT_INDEX index, OMX_BUFFERHEADERTYPE* pBuffer, bool bFlush) = 0;
    virtual OMX_ERRORTYPE reportOutputCrop() = 0;
};

class ISVProcessor : public Thread
{
public:
    ISVProcessor(bool canCallJava, sp<ISVBufferManager> bufferManager, sp<ISVProcessorObserver> observer, uint32_t width, uint32_t height);
    virtual ~ISVProcessor();

    virtual status_t readyToRun();

    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
                                                // 1) loop: if threadLoop() returns true, it will be called again if
                                                //          requestExit() wasn't called.
                                                // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool threadLoop();
    bool isCurrentThread() const;

    void start();
    void stop();
    bool isReadytoRun();

    //configure FRC factor
    status_t configFRC(uint32_t fps);
    //add output buffer into mOutputBuffers
    void addOutput(OMX_BUFFERHEADERTYPE* output);
    //add intput buffer into mInputBuffers
    void addInput(OMX_BUFFERHEADERTYPE* input);
    //notify flush and wait flush finish
    void notifyFlush();
    void waitFlushFinished();

private:
    bool getBufForFirmwareOutput(Vector<ISVBuffer*> *fillBufList,
            uint32_t *fillBufNum);
    status_t updateFirmwareOutputBufStatus(uint32_t fillBufNum);
    bool getBufForFirmwareInput(Vector<ISVBuffer*> *procBufList,
            ISVBuffer **inputBuf,
            uint32_t *procBufNum );
    status_t updateFirmwareInputBufStatus(uint32_t procBufNum);
    //flush input&ouput buffer queue
    void flush();
    //return whether this fps is valid
    inline bool isFrameRateValid(uint32_t fps);
    //auto calculate fps if the framework doesn't set the correct fps
    status_t calculateFps(int64_t timeStamp, uint32_t* fps);
    //config vpp filters
    status_t configFilters(OMX_BUFFERHEADERTYPE* buffer);

private:
    sp<ISVProcessorObserver> mpOwner;
    android_thread_id_t mThreadId;
    bool mThreadRunning;

    sp<ISVWorker> mISVWorker;
    sp<ISVProfile> mISVProfile;
    // buffer manager
    sp<ISVBufferManager> mBufferManager;

    Vector<OMX_BUFFERHEADERTYPE*> mOutputBuffers;
    Mutex mOutputLock; // to protect access to mOutputBuffers
    uint32_t mOutputProcIdx;

    Vector<OMX_BUFFERHEADERTYPE*> mInputBuffers;
    Mutex mInputLock; // to protect access to mFillBuffers
    uint32_t mInputProcIdx;

    const static uint32_t WINDOW_SIZE = 4;  // must >= 2
    Vector<int64_t>mTimeWindow;
    // conditon for thread running
    Mutex mLock;
    Condition mRunCond;

    // condition for seek finish
    Mutex mEndLock;
    Condition mEndCond;

    uint32_t mNumTaskInProcesing;
    uint32_t mNumRetry;
    bool mError;
    bool mbFlush;
    bool mbBypass;
    bool mFlagEnd;
    bool mFrcOn;

    // ISV filter configuration
    uint32_t mFilters;
    FilterParam mFilterParam;
};

#endif /* __ISV_THREAD_H*/
