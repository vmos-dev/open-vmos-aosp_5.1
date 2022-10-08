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

#include <string.h>
#include "VideoEncoderLog.h"
#include "VideoEncoderBase.h"
#include "IntelMetadataBuffer.h"
#include <va/va_tpi.h>
#include <va/va_android.h>

VideoEncoderBase::VideoEncoderBase()
    :mInitialized(true)
    ,mStarted(false)
    ,mVADisplay(NULL)
    ,mVAContext(VA_INVALID_ID)
    ,mVAConfig(VA_INVALID_ID)
    ,mVAEntrypoint(VAEntrypointEncSlice)
    ,mNewHeader(false)
    ,mRenderMaxSliceSize(false)
    ,mRenderQP (false)
    ,mRenderAIR(false)
    ,mRenderCIR(false)
    ,mRenderFrameRate(false)
    ,mRenderBitRate(false)
    ,mRenderHrd(false)
    ,mRenderMultiTemporal(false)
    ,mForceKFrame(false)
    ,mSeqParamBuf(0)
    ,mPicParamBuf(0)
    ,mSliceParamBuf(0)
    ,mAutoRefSurfaces(NULL)
    ,mRefSurface(VA_INVALID_SURFACE)
    ,mRecSurface(VA_INVALID_SURFACE)
    ,mFrameNum(0)
    ,mCodedBufSize(0)
    ,mAutoReference(false)
    ,mAutoReferenceSurfaceNum(4)
    ,mEncPackedHeaders(VA_ATTRIB_NOT_SUPPORTED)
    ,mSliceSizeOverflow(false)
    ,mCurOutputTask(NULL)
    ,mOutCodedBuffer(0)
    ,mOutCodedBufferPtr(NULL)
    ,mCurSegment(NULL)
    ,mOffsetInSeg(0)
    ,mTotalSize(0)
    ,mTotalSizeCopied(0)
    ,mFrameSkipped(false)
    ,mSupportedSurfaceMemType(0)
    ,mVASurfaceMappingAction(0)
#ifdef INTEL_VIDEO_XPROC_SHARING
    ,mSessionFlag(0)
#endif
    {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    // here the display can be any value, use following one
    // just for consistence purpose, so don't define it
    unsigned int display = 0x18C34078;
    int majorVersion = -1;
    int minorVersion = -1;

    setDefaultParams();

    LOG_V("vaGetDisplay \n");
    mVADisplay = vaGetDisplay(&display);
    if (mVADisplay == NULL) {
        LOG_E("vaGetDisplay failed.");
    }

    vaStatus = vaInitialize(mVADisplay, &majorVersion, &minorVersion);
    LOG_V("vaInitialize \n");
    if (vaStatus != VA_STATUS_SUCCESS) {
        LOG_E( "Failed vaInitialize, vaStatus = %d\n", vaStatus);
        mInitialized = false;
    }
}

VideoEncoderBase::~VideoEncoderBase() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    stop();

    vaStatus = vaTerminate(mVADisplay);
    LOG_V( "vaTerminate\n");
    if (vaStatus != VA_STATUS_SUCCESS) {
        LOG_W( "Failed vaTerminate, vaStatus = %d\n", vaStatus);
    } else {
        mVADisplay = NULL;
    }

#ifdef INTEL_VIDEO_XPROC_SHARING
    IntelMetadataBuffer::ClearContext(mSessionFlag, false);
#endif
}

Encode_Status VideoEncoderBase::start() {

    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    if (!mInitialized) {
        LOGE("Encoder Initialize fail can not start");
        return ENCODE_DRIVER_FAIL;
    }

    if (mStarted) {
        LOG_V("Encoder has been started\n");
        return ENCODE_ALREADY_INIT;
    }

    if (mComParams.rawFormat != RAW_FORMAT_NV12)
#ifdef IMG_GFX
        mVASurfaceMappingAction |= MAP_ACTION_COLORCONVERT;
#else
        return ENCODE_NOT_SUPPORTED;
#endif

    if (mComParams.resolution.width > 2048 || mComParams.resolution.height > 2048){
        LOGE("Unsupported resolution width %d, height %d\n",
            mComParams.resolution.width, mComParams.resolution.height);
        return ENCODE_NOT_SUPPORTED;
    }
    queryAutoReferenceConfig(mComParams.profile);

    VAConfigAttrib vaAttrib_tmp[6],vaAttrib[VAConfigAttribTypeMax];
    int vaAttribNumber = 0;
    vaAttrib_tmp[0].type = VAConfigAttribRTFormat;
    vaAttrib_tmp[1].type = VAConfigAttribRateControl;
    vaAttrib_tmp[2].type = VAConfigAttribEncAutoReference;
    vaAttrib_tmp[3].type = VAConfigAttribEncPackedHeaders;
    vaAttrib_tmp[4].type = VAConfigAttribEncMaxRefFrames;
    vaAttrib_tmp[5].type = VAConfigAttribEncRateControlExt;

    vaStatus = vaGetConfigAttributes(mVADisplay, mComParams.profile,
            VAEntrypointEncSlice, &vaAttrib_tmp[0], 6);
    CHECK_VA_STATUS_RETURN("vaGetConfigAttributes");

    if((vaAttrib_tmp[0].value & VA_RT_FORMAT_YUV420) != 0)
    {
        vaAttrib[vaAttribNumber].type = VAConfigAttribRTFormat;
        vaAttrib[vaAttribNumber].value = VA_RT_FORMAT_YUV420;
        vaAttribNumber++;
    }

    vaAttrib[vaAttribNumber].type = VAConfigAttribRateControl;
    vaAttrib[vaAttribNumber].value = mComParams.rcMode;
    vaAttribNumber++;

    vaAttrib[vaAttribNumber].type = VAConfigAttribEncAutoReference;
    vaAttrib[vaAttribNumber].value = mAutoReference ? 1 : VA_ATTRIB_NOT_SUPPORTED;
    vaAttribNumber++;

    if(vaAttrib_tmp[3].value != VA_ATTRIB_NOT_SUPPORTED)
    {
        vaAttrib[vaAttribNumber].type = VAConfigAttribEncPackedHeaders;
        vaAttrib[vaAttribNumber].value = vaAttrib[3].value;
        vaAttribNumber++;
        mEncPackedHeaders = vaAttrib[3].value;
    }

    if(vaAttrib_tmp[4].value != VA_ATTRIB_NOT_SUPPORTED)
    {
        vaAttrib[vaAttribNumber].type = VAConfigAttribEncMaxRefFrames;
        vaAttrib[vaAttribNumber].value = vaAttrib[4].value;
        vaAttribNumber++;
        mEncMaxRefFrames = vaAttrib[4].value;
    }

    if(vaAttrib_tmp[5].value != VA_ATTRIB_NOT_SUPPORTED)
    {
        vaAttrib[vaAttribNumber].type = VAConfigAttribEncRateControlExt;
        vaAttrib[vaAttribNumber].value = mComParams.numberOfLayer;
        vaAttribNumber++;
    }

    LOG_V( "======VA Configuration======\n");
    LOG_V( "profile = %d\n", mComParams.profile);
    LOG_V( "mVAEntrypoint = %d\n", mVAEntrypoint);
    LOG_V( "vaAttrib[0].type = %d\n", vaAttrib[0].type);
    LOG_V( "vaAttrib[1].type = %d\n", vaAttrib[1].type);
    LOG_V( "vaAttrib[2].type = %d\n", vaAttrib[2].type);
    LOG_V( "vaAttrib[0].value (Format) = %d\n", vaAttrib[0].value);
    LOG_V( "vaAttrib[1].value (RC mode) = %d\n", vaAttrib[1].value);
    LOG_V( "vaAttrib[2].value (AutoReference) = %d\n", vaAttrib[2].value);
    LOG_V( "vaAttribNumber is %d\n", vaAttribNumber);
    LOG_V( "mComParams.numberOfLayer is %d\n", mComParams.numberOfLayer);

    LOG_V( "vaCreateConfig\n");

    vaStatus = vaCreateConfig(
            mVADisplay, mComParams.profile, mVAEntrypoint,
            &vaAttrib[0], vaAttribNumber, &(mVAConfig));
//            &vaAttrib[0], 3, &(mVAConfig));  //uncomment this after psb_video supports
    CHECK_VA_STATUS_RETURN("vaCreateConfig");

    querySupportedSurfaceMemTypes();

    if (mComParams.rcMode == VA_RC_VCM) {
        // Following three features are only enabled in VCM mode
        mRenderMaxSliceSize = true;
        mRenderAIR = true;
        mRenderBitRate = true;
    }

    LOG_V( "======VA Create Surfaces for Rec/Ref frames ======\n");

    uint32_t stride_aligned, height_aligned;
    if(mAutoReference == false){
        stride_aligned = (mComParams.resolution.width + 15) & ~15;
        height_aligned = (mComParams.resolution.height + 15) & ~15;
    }else{
        // this alignment is used for AVC. For vp8 encode, driver will handle the alignment
        if(mComParams.profile == VAProfileVP8Version0_3)
        {
            stride_aligned = mComParams.resolution.width;
            height_aligned = mComParams.resolution.height;
            mVASurfaceMappingAction |= MAP_ACTION_COPY;
        }
        else
        {
            stride_aligned = (mComParams.resolution.width + 63) & ~63;  //on Merr, stride must be 64 aligned.
            height_aligned = (mComParams.resolution.height + 31) & ~31;
            mVASurfaceMappingAction |= MAP_ACTION_ALIGN64;
        }
    }

    if(mAutoReference == false){
        mRefSurface = CreateNewVASurface(mVADisplay, stride_aligned, height_aligned);
        mRecSurface = CreateNewVASurface(mVADisplay, stride_aligned, height_aligned);

    }else {
        mAutoRefSurfaces = new VASurfaceID [mAutoReferenceSurfaceNum];
        for(uint32_t i = 0; i < mAutoReferenceSurfaceNum; i ++)
            mAutoRefSurfaces[i] = CreateNewVASurface(mVADisplay, stride_aligned, height_aligned);
    }
    CHECK_VA_STATUS_RETURN("vaCreateSurfaces");

    //Prepare all Surfaces to be added into Context
    uint32_t contextSurfaceCnt;
    if(mAutoReference == false )
        contextSurfaceCnt = 2 + mSrcSurfaceMapList.size();
    else
        contextSurfaceCnt = mAutoReferenceSurfaceNum + mSrcSurfaceMapList.size();

    VASurfaceID *contextSurfaces = new VASurfaceID[contextSurfaceCnt];
    int32_t index = -1;
    android::List<VASurfaceMap *>::iterator map_node;

    for(map_node = mSrcSurfaceMapList.begin(); map_node !=  mSrcSurfaceMapList.end(); map_node++)
    {
        contextSurfaces[++index] = (*map_node)->getVASurface();
        (*map_node)->setTracked();
    }

    if(mAutoReference == false){
        contextSurfaces[++index] = mRefSurface;
        contextSurfaces[++index] = mRecSurface;
    } else {
        for (uint32_t i=0; i < mAutoReferenceSurfaceNum; i++)
            contextSurfaces[++index] = mAutoRefSurfaces[i];
    }

    //Initialize and save the VA context ID
    LOG_V( "vaCreateContext\n");
    vaStatus = vaCreateContext(mVADisplay, mVAConfig,
#ifdef IMG_GFX
            mComParams.resolution.width,
            mComParams.resolution.height,
#else
            stride_aligned,
            height_aligned,
#endif
            VA_PROGRESSIVE, contextSurfaces, contextSurfaceCnt,
            &(mVAContext));
    CHECK_VA_STATUS_RETURN("vaCreateContext");

    delete [] contextSurfaces;

    LOG_I("Success to create libva context width %d, height %d\n",
          mComParams.resolution.width, mComParams.resolution.height);

    uint32_t maxSize = 0;
    ret = getMaxOutSize(&maxSize);
    CHECK_ENCODE_STATUS_RETURN("getMaxOutSize");

    // Create CodedBuffer for output
    VABufferID VACodedBuffer;

    for(uint32_t i = 0; i <mComParams.codedBufNum; i++) {
            vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                    VAEncCodedBufferType,
                    mCodedBufSize,
                    1, NULL,
                    &VACodedBuffer);
            CHECK_VA_STATUS_RETURN("vaCreateBuffer::VAEncCodedBufferType");

            mVACodedBufferList.push_back(VACodedBuffer);
    }

    if (ret == ENCODE_SUCCESS)
        mStarted = true;

    LOG_V( "end\n");
    return ret;
}

Encode_Status VideoEncoderBase::encode(VideoEncRawBuffer *inBuffer, uint32_t timeout) {

    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    if (!mStarted) {
        LOG_E("Encoder has not initialized yet\n");
        return ENCODE_NOT_INIT;
    }

    CHECK_NULL_RETURN_IFFAIL(inBuffer);

    //======Prepare all resources encoder needed=====.

    //Prepare encode vaSurface
    VASurfaceID sid = VA_INVALID_SURFACE;
    ret = manageSrcSurface(inBuffer, &sid);
    CHECK_ENCODE_STATUS_RETURN("manageSrcSurface");

    //Prepare CodedBuffer
    mCodedBuffer_Lock.lock();
    if(mVACodedBufferList.empty()){
        if(timeout == FUNC_BLOCK)
            mCodedBuffer_Cond.wait(mCodedBuffer_Lock);
        else if (timeout > 0) {
            if(NO_ERROR != mEncodeTask_Cond.waitRelative(mCodedBuffer_Lock, 1000000*timeout)){
                mCodedBuffer_Lock.unlock();
                LOG_E("Time out wait for Coded buffer.\n");
                return ENCODE_DEVICE_BUSY;
            }
        }
        else {//Nonblock
            mCodedBuffer_Lock.unlock();
            LOG_E("Coded buffer is not ready now.\n");
            return ENCODE_DEVICE_BUSY;
        }
    }

    if(mVACodedBufferList.empty()){
        mCodedBuffer_Lock.unlock();
        return ENCODE_DEVICE_BUSY;
    }
    VABufferID coded_buf = (VABufferID) *(mVACodedBufferList.begin());
    mVACodedBufferList.erase(mVACodedBufferList.begin());
    mCodedBuffer_Lock.unlock();

    LOG_V("CodedBuffer ID 0x%08x\n", coded_buf);

    //All resources are ready, start to assemble EncodeTask
    EncodeTask* task = new EncodeTask();

    task->completed = false;
    task->enc_surface = sid;
    task->coded_buffer = coded_buf;
    task->timestamp = inBuffer->timeStamp;
    task->priv = inBuffer->priv;

    //Setup frame info, like flag ( SYNCFRAME), frame number, type etc
    task->type = inBuffer->type;
    task->flag = inBuffer->flag;
    PrepareFrameInfo(task);

    if(mAutoReference == false){
        //Setup ref /rec frames
        //TODO: B frame support, temporary use same logic
        switch (inBuffer->type) {
            case FTYPE_UNKNOWN:
            case FTYPE_IDR:
            case FTYPE_I:
            case FTYPE_P:
            {
                if(!mFrameSkipped) {
                    VASurfaceID tmpSurface = mRecSurface;
                    mRecSurface = mRefSurface;
                    mRefSurface = tmpSurface;
                }

                task->ref_surface = mRefSurface;
                task->rec_surface = mRecSurface;

                break;
            }
            case FTYPE_B:
            default:
                LOG_V("Something wrong, B frame may not be supported in this mode\n");
                ret = ENCODE_NOT_SUPPORTED;
                goto CLEAN_UP;
        }
    }else {
        task->ref_surface = VA_INVALID_SURFACE;
        task->rec_surface = VA_INVALID_SURFACE;
    }
    //======Start Encoding, add task to list======
    LOG_V("Start Encoding vaSurface=0x%08x\n", task->enc_surface);

    vaStatus = vaBeginPicture(mVADisplay, mVAContext, task->enc_surface);
    CHECK_VA_STATUS_GOTO_CLEANUP("vaBeginPicture");

    ret = sendEncodeCommand(task);
    CHECK_ENCODE_STATUS_CLEANUP("sendEncodeCommand");

    vaStatus = vaEndPicture(mVADisplay, mVAContext);
    CHECK_VA_STATUS_GOTO_CLEANUP("vaEndPicture");

    LOG_V("Add Task %p into Encode Task list\n", task);
    mEncodeTask_Lock.lock();
    mEncodeTaskList.push_back(task);
    mEncodeTask_Cond.signal();
    mEncodeTask_Lock.unlock();

    mFrameNum ++;

    LOG_V("encode return Success\n");

    return ENCODE_SUCCESS;

CLEAN_UP:

    delete task;
    mCodedBuffer_Lock.lock();
    mVACodedBufferList.push_back(coded_buf); //push to CodedBuffer pool again since it is not used
    mCodedBuffer_Cond.signal();
    mCodedBuffer_Lock.unlock();

    LOG_V("encode return error=%x\n", ret);

    return ret;
}

/*
  1. Firstly check if one task is outputting data, if yes, continue outputting, if not try to get one from list.
  2. Due to block/non-block/block with timeout 3 modes, if task is not completed, then sync surface, if yes,
    start output data
  3. Use variable curoutputtask to record task which is getOutput() working on to avoid push again when get failure
    on non-block/block with timeout modes.
  4. if complete all output data, curoutputtask should be set NULL
*/
Encode_Status VideoEncoderBase::getOutput(VideoEncOutputBuffer *outBuffer, uint32_t timeout) {

    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    bool useLocalBuffer = false;

    CHECK_NULL_RETURN_IFFAIL(outBuffer);

    if (mCurOutputTask == NULL) {
        mEncodeTask_Lock.lock();
        if(mEncodeTaskList.empty()) {
            LOG_V("getOutput CurrentTask is NULL\n");
            if(timeout == FUNC_BLOCK) {
                LOG_V("waiting for task....\n");
                mEncodeTask_Cond.wait(mEncodeTask_Lock);
            } else if (timeout > 0) {
                LOG_V("waiting for task in %i ms....\n", timeout);
                if(NO_ERROR != mEncodeTask_Cond.waitRelative(mEncodeTask_Lock, 1000000*timeout)) {
                    mEncodeTask_Lock.unlock();
                    LOG_E("Time out wait for encode task.\n");
                    return ENCODE_NO_REQUEST_DATA;
                }
            } else {//Nonblock
                mEncodeTask_Lock.unlock();
                return ENCODE_NO_REQUEST_DATA;
            }
        }

        if(mEncodeTaskList.empty()){
            mEncodeTask_Lock.unlock();
            return ENCODE_DATA_NOT_READY;
        }
        mCurOutputTask =  *(mEncodeTaskList.begin());
        mEncodeTaskList.erase(mEncodeTaskList.begin());
        mEncodeTask_Lock.unlock();
    }

    //sync/query/wait task if not completed
    if (mCurOutputTask->completed == false) {
        VASurfaceStatus vaSurfaceStatus;

        if (timeout == FUNC_BLOCK) {
            //block mode, direct sync surface to output data

            mOutCodedBuffer = mCurOutputTask->coded_buffer;

            // Check frame skip
            // Need encoding to be completed before calling query surface below to
            // get the right skip frame flag for current frame
            // It is a requirement of video driver
            // vaSyncSurface syncs the wrong frame when rendering the same surface multiple times,
            // so use vaMapbuffer instead
            LOG_V ("block mode, vaMapBuffer ID = 0x%08x\n", mOutCodedBuffer);
            if (mOutCodedBufferPtr == NULL) {
                vaStatus = vaMapBuffer (mVADisplay, mOutCodedBuffer, (void **)&mOutCodedBufferPtr);
                CHECK_VA_STATUS_GOTO_CLEANUP("vaMapBuffer");
                CHECK_NULL_RETURN_IFFAIL(mOutCodedBufferPtr);
            }

            vaStatus = vaQuerySurfaceStatus(mVADisplay, mCurOutputTask->enc_surface,  &vaSurfaceStatus);
            CHECK_VA_STATUS_RETURN("vaQuerySurfaceStatus");
            mFrameSkipped = vaSurfaceStatus & VASurfaceSkipped;

            mCurOutputTask->completed = true;

        } else {
            //For both block with timeout and non-block mode, query surface, if ready, output data
            LOG_V ("non-block mode, vaQuerySurfaceStatus ID = 0x%08x\n", mCurOutputTask->enc_surface);

            vaStatus = vaQuerySurfaceStatus(mVADisplay, mCurOutputTask->enc_surface,  &vaSurfaceStatus);
            if (vaSurfaceStatus & VASurfaceReady) {
                mOutCodedBuffer = mCurOutputTask->coded_buffer;
                mFrameSkipped = vaSurfaceStatus & VASurfaceSkipped;
                mCurOutputTask->completed = true;
                //if need to call SyncSurface again ?

            }  else {//not encode complet yet, but keep all context and return directly
                return ENCODE_DATA_NOT_READY;
            }

        }

    }

    //start to output data
    ret = prepareForOutput(outBuffer, &useLocalBuffer);
    CHECK_ENCODE_STATUS_CLEANUP("prepareForOutput");

    //copy all flags to outBuffer
    outBuffer->offset = 0;
    outBuffer->flag = mCurOutputTask->flag;
    outBuffer->type = mCurOutputTask->type;
    outBuffer->timeStamp = mCurOutputTask->timestamp;
    outBuffer->priv = mCurOutputTask->priv;

    if (outBuffer->format == OUTPUT_EVERYTHING || outBuffer->format == OUTPUT_FRAME_DATA) {
        ret = outputAllData(outBuffer);
        CHECK_ENCODE_STATUS_CLEANUP("outputAllData");
    }else {
        ret = getExtFormatOutput(outBuffer);
        CHECK_ENCODE_STATUS_CLEANUP("getExtFormatOutput");
    }

    LOG_V("out size for this getOutput call = %d\n", outBuffer->dataSize);

    ret = cleanupForOutput();
    CHECK_ENCODE_STATUS_CLEANUP("cleanupForOutput");

    LOG_V("getOutput return Success, Frame skip is %d\n", mFrameSkipped);

    return ENCODE_SUCCESS;

CLEAN_UP:

    if (outBuffer->data && (useLocalBuffer == true)) {
        delete[] outBuffer->data;
        outBuffer->data = NULL;
        useLocalBuffer = false;
    }

    if (mOutCodedBufferPtr != NULL) {
        vaStatus = vaUnmapBuffer(mVADisplay, mOutCodedBuffer);
        mOutCodedBufferPtr = NULL;
        mCurSegment = NULL;
    }

    delete mCurOutputTask;
    mCurOutputTask = NULL;
    mCodedBuffer_Lock.lock();
    mVACodedBufferList.push_back(mOutCodedBuffer);
    mCodedBuffer_Cond.signal();
    mCodedBuffer_Lock.unlock();

    LOG_V("getOutput return error=%x\n", ret);
    return ret;
}

void VideoEncoderBase::flush() {

    LOG_V( "Begin\n");

    // reset the properities
    mFrameNum = 0;

    LOG_V( "end\n");
}

Encode_Status VideoEncoderBase::stop() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    Encode_Status ret = ENCODE_SUCCESS;

    LOG_V( "Begin\n");

    // It is possible that above pointers have been allocated
    // before we set mStarted to true
    if (!mStarted) {
        LOG_V("Encoder has been stopped\n");
        return ENCODE_SUCCESS;
    }
    if (mAutoRefSurfaces) {
        delete[] mAutoRefSurfaces;
        mAutoRefSurfaces = NULL;
    }

    mCodedBuffer_Lock.lock();
    mVACodedBufferList.clear();
    mCodedBuffer_Lock.unlock();
    mCodedBuffer_Cond.broadcast();

    //Delete all uncompleted tasks
    mEncodeTask_Lock.lock();
    while(! mEncodeTaskList.empty())
    {
        delete *mEncodeTaskList.begin();
        mEncodeTaskList.erase(mEncodeTaskList.begin());
    }
    mEncodeTask_Lock.unlock();
    mEncodeTask_Cond.broadcast();

    //Release Src Surface Buffer Map, destroy surface manually since it is not added into context
    LOG_V( "Rlease Src Surface Map\n");
    while(! mSrcSurfaceMapList.empty())
    {
        delete (*mSrcSurfaceMapList.begin());
        mSrcSurfaceMapList.erase(mSrcSurfaceMapList.begin());
    }

    LOG_V( "vaDestroyContext\n");
    if (mVAContext != VA_INVALID_ID) {
        vaStatus = vaDestroyContext(mVADisplay, mVAContext);
        CHECK_VA_STATUS_GOTO_CLEANUP("vaDestroyContext");
    }

    LOG_V( "vaDestroyConfig\n");
    if (mVAConfig != VA_INVALID_ID) {
        vaStatus = vaDestroyConfig(mVADisplay, mVAConfig);
        CHECK_VA_STATUS_GOTO_CLEANUP("vaDestroyConfig");
    }

CLEAN_UP:

    mStarted = false;
    mSliceSizeOverflow = false;
    mCurOutputTask= NULL;
    mOutCodedBuffer = 0;
    mCurSegment = NULL;
    mOffsetInSeg =0;
    mTotalSize = 0;
    mTotalSizeCopied = 0;
    mFrameSkipped = false;
    mSupportedSurfaceMemType = 0;

    LOG_V( "end\n");
    return ret;
}

Encode_Status VideoEncoderBase::prepareForOutput(
        VideoEncOutputBuffer *outBuffer, bool *useLocalBuffer) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VACodedBufferSegment *vaCodedSeg = NULL;
    uint32_t status = 0;

    LOG_V( "begin\n");
    // Won't check parameters here as the caller already checked them
    // mCurSegment is NULL means it is first time to be here after finishing encoding a frame
    if (mCurSegment == NULL) {
        if (mOutCodedBufferPtr == NULL) {
            vaStatus = vaMapBuffer (mVADisplay, mOutCodedBuffer, (void **)&mOutCodedBufferPtr);
            CHECK_VA_STATUS_RETURN("vaMapBuffer");
            CHECK_NULL_RETURN_IFFAIL(mOutCodedBufferPtr);
        }

        LOG_V("Coded Buffer ID been mapped = 0x%08x\n", mOutCodedBuffer);

        mTotalSize = 0;
        mOffsetInSeg = 0;
        mTotalSizeCopied = 0;
        vaCodedSeg = (VACodedBufferSegment *)mOutCodedBufferPtr;
        mCurSegment = (VACodedBufferSegment *)mOutCodedBufferPtr;

        while (1) {

            mTotalSize += vaCodedSeg->size;
            status = vaCodedSeg->status;
#ifndef IMG_GFX
            uint8_t *pTemp;
            uint32_t ii;
            pTemp = (uint8_t*)vaCodedSeg->buf;
            for(ii = 0; ii < 16;){
                if (*(pTemp + ii) == 0xFF)
                    ii++;
                else
                    break;
            }
            if (ii > 0) {
                mOffsetInSeg = ii;
            }
#endif
            if (!mSliceSizeOverflow) {
                mSliceSizeOverflow = status & VA_CODED_BUF_STATUS_SLICE_OVERFLOW_MASK;
            }

            if (vaCodedSeg->next == NULL)
                break;

            vaCodedSeg = (VACodedBufferSegment *)vaCodedSeg->next;
        }
    }

    // We will support two buffer allocation mode,
    // one is application allocates the buffer and passes to encode,
    // the other is encode allocate memory

    //means  app doesn't allocate the buffer, so _encode will allocate it.
    if (outBuffer->data == NULL) {
        *useLocalBuffer = true;
        outBuffer->data = new  uint8_t[mTotalSize - mTotalSizeCopied + 100];
        if (outBuffer->data == NULL) {
            LOG_E( "outBuffer->data == NULL\n");
            return ENCODE_NO_MEMORY;
        }
        outBuffer->bufferSize = mTotalSize + 100;
        outBuffer->dataSize = 0;
    }

    // Clear all flag for every call
    outBuffer->flag = 0;
    if (mSliceSizeOverflow) outBuffer->flag |= ENCODE_BUFFERFLAG_SLICEOVERFOLOW;

    if (!mCurSegment)
        return ENCODE_FAIL;

    if (mCurSegment->size < mOffsetInSeg) {
        LOG_E("mCurSegment->size < mOffsetInSeg\n");
        return ENCODE_FAIL;
    }

    // Make sure we have data in current segment
    if (mCurSegment->size == mOffsetInSeg) {
        if (mCurSegment->next != NULL) {
            mCurSegment = (VACodedBufferSegment *)mCurSegment->next;
            mOffsetInSeg = 0;
        } else {
            LOG_V("No more data available\n");
            outBuffer->flag |= ENCODE_BUFFERFLAG_DATAINVALID;
            outBuffer->dataSize = 0;
            mCurSegment = NULL;
            return ENCODE_NO_REQUEST_DATA;
        }
    }

    LOG_V( "end\n");
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::cleanupForOutput() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    //mCurSegment is NULL means all data has been copied out
    if (mCurSegment == NULL && mOutCodedBufferPtr) {
        vaStatus = vaUnmapBuffer(mVADisplay, mOutCodedBuffer);
        CHECK_VA_STATUS_RETURN("vaUnmapBuffer");
        mOutCodedBufferPtr = NULL;
        mTotalSize = 0;
        mOffsetInSeg = 0;
        mTotalSizeCopied = 0;

        delete mCurOutputTask;
        mCurOutputTask = NULL;
        mCodedBuffer_Lock.lock();
        mVACodedBufferList.push_back(mOutCodedBuffer);
        mCodedBuffer_Cond.signal();
        mCodedBuffer_Lock.unlock();

        LOG_V("All data has been outputted, return CodedBuffer 0x%08x to pool\n", mOutCodedBuffer);
    }
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::queryProfileLevelConfig(VADisplay dpy, VAProfile profile) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEntrypoint entryPtr[8];
    int i, entryPtrNum;

    if(profile ==  VAProfileH264Main) //need to be fixed
        return ENCODE_NOT_SUPPORTED;

    vaStatus = vaQueryConfigEntrypoints(dpy, profile, entryPtr, &entryPtrNum);
    CHECK_VA_STATUS_RETURN("vaQueryConfigEntrypoints");

    for(i=0; i<entryPtrNum; i++){
        if(entryPtr[i] == VAEntrypointEncSlice)
            return ENCODE_SUCCESS;
    }

    return ENCODE_NOT_SUPPORTED;
}

Encode_Status VideoEncoderBase::queryAutoReferenceConfig(VAProfile profile) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAConfigAttrib attrib_list;
    attrib_list.type = VAConfigAttribEncAutoReference;
    attrib_list.value = VA_ATTRIB_NOT_SUPPORTED;

    vaStatus = vaGetConfigAttributes(mVADisplay, profile, VAEntrypointEncSlice, &attrib_list, 1);
    if(attrib_list.value == VA_ATTRIB_NOT_SUPPORTED )
        mAutoReference = false;
    else
        mAutoReference = true;

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::querySupportedSurfaceMemTypes() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    unsigned int num = 0;

    VASurfaceAttrib* attribs = NULL;

    //get attribs number
    vaStatus = vaQuerySurfaceAttributes(mVADisplay, mVAConfig, attribs, &num);
    CHECK_VA_STATUS_RETURN("vaGetSurfaceAttributes");

    if (num == 0)
        return ENCODE_SUCCESS;

    attribs = new VASurfaceAttrib[num];

    vaStatus = vaQuerySurfaceAttributes(mVADisplay, mVAConfig, attribs, &num);
    CHECK_VA_STATUS_RETURN("vaGetSurfaceAttributes");

    for(uint32_t i = 0; i < num; i ++) {
        if (attribs[i].type == VASurfaceAttribMemoryType) {
            mSupportedSurfaceMemType = attribs[i].value.value.i;
            break;
        }
        else
            continue;
    }

    delete attribs;

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::outputAllData(VideoEncOutputBuffer *outBuffer) {

    // Data size been copied for every single call
    uint32_t sizeCopiedHere = 0;
    uint32_t sizeToBeCopied = 0;

    CHECK_NULL_RETURN_IFFAIL(outBuffer->data);

    while (1) {

        LOG_V("mCurSegment->size = %d, mOffsetInSeg = %d\n", mCurSegment->size, mOffsetInSeg);
        LOG_V("outBuffer->bufferSize = %d, sizeCopiedHere = %d, mTotalSizeCopied = %d\n",
              outBuffer->bufferSize, sizeCopiedHere, mTotalSizeCopied);

        if (mCurSegment->size < mOffsetInSeg || outBuffer->bufferSize < sizeCopiedHere) {
            LOG_E("mCurSegment->size < mOffsetInSeg  || outBuffer->bufferSize < sizeCopiedHere\n");
            return ENCODE_FAIL;
        }

        if ((mCurSegment->size - mOffsetInSeg) <= outBuffer->bufferSize - sizeCopiedHere) {
            sizeToBeCopied = mCurSegment->size - mOffsetInSeg;
            memcpy(outBuffer->data + sizeCopiedHere,
                   (uint8_t *)mCurSegment->buf + mOffsetInSeg, sizeToBeCopied);
            sizeCopiedHere += sizeToBeCopied;
            mTotalSizeCopied += sizeToBeCopied;
            mOffsetInSeg = 0;
        } else {
            sizeToBeCopied = outBuffer->bufferSize - sizeCopiedHere;
            memcpy(outBuffer->data + sizeCopiedHere,
                   (uint8_t *)mCurSegment->buf + mOffsetInSeg, outBuffer->bufferSize - sizeCopiedHere);
            mTotalSizeCopied += sizeToBeCopied;
            mOffsetInSeg += sizeToBeCopied;
            outBuffer->dataSize = outBuffer->bufferSize;
            outBuffer->remainingSize = mTotalSize - mTotalSizeCopied;
            outBuffer->flag |= ENCODE_BUFFERFLAG_PARTIALFRAME;
            return ENCODE_BUFFER_TOO_SMALL;
        }

        if (mCurSegment->next == NULL) {
            outBuffer->dataSize = sizeCopiedHere;
            outBuffer->remainingSize = 0;
            outBuffer->flag |= ENCODE_BUFFERFLAG_ENDOFFRAME;
            mCurSegment = NULL;
            return ENCODE_SUCCESS;
        }

        mCurSegment = (VACodedBufferSegment *)mCurSegment->next;
        mOffsetInSeg = 0;
    }
}

void VideoEncoderBase::setDefaultParams() {

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
    mComParams.rcParams.maxQP = 0;
    mComParams.rcParams.I_minQP = 0;
    mComParams.rcParams.I_maxQP = 0;
    mComParams.rcParams.bitRate = 640000;
    mComParams.rcParams.targetPercentage= 0;
    mComParams.rcParams.windowSize = 0;
    mComParams.rcParams.disableFrameSkip = 0;
    mComParams.rcParams.disableBitsStuffing = 1;
    mComParams.rcParams.enableIntraFrameQPControl = 0;
    mComParams.rcParams.temporalFrameRate = 0;
    mComParams.rcParams.temporalID = 0;
    mComParams.cyclicFrameInterval = 30;
    mComParams.refreshType = VIDEO_ENC_NONIR;
    mComParams.airParams.airMBs = 0;
    mComParams.airParams.airThreshold = 0;
    mComParams.airParams.airAuto = 1;
    mComParams.disableDeblocking = 2;
    mComParams.syncEncMode = false;
    mComParams.codedBufNum = 2;
    mComParams.numberOfLayer = 1;
    mComParams.nPeriodicity = 0;
    memset(mComParams.nLayerID,0,32*sizeof(uint32_t));

    mHrdParam.bufferSize = 0;
    mHrdParam.initBufferFullness = 0;

    mStoreMetaDataInBuffers.isEnabled = false;
}

Encode_Status VideoEncoderBase::setParameters(
        VideoParamConfigSet *videoEncParams) {

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(videoEncParams);
    LOG_V("Config type = %x\n", (int)videoEncParams->type);

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
            break;
        }

        case VideoParamsTypeUpSteamBuffer: {

            VideoParamsUpstreamBuffer *upStreamBuffer =
                    reinterpret_cast <VideoParamsUpstreamBuffer *> (videoEncParams);

            if (upStreamBuffer->size != sizeof (VideoParamsUpstreamBuffer)) {
                return ENCODE_INVALID_PARAMS;
            }

            ret = setUpstreamBuffer(upStreamBuffer);
            break;
        }

        case VideoParamsTypeUsrptrBuffer: {

            // usrptr only can be get
            // this case should not happen
            break;
        }

        case VideoParamsTypeHRD: {
            VideoParamsHRD *hrd =
                    reinterpret_cast <VideoParamsHRD *> (videoEncParams);

            if (hrd->size != sizeof (VideoParamsHRD)) {
                return ENCODE_INVALID_PARAMS;
            }

            mHrdParam.bufferSize = hrd->bufferSize;
            mHrdParam.initBufferFullness = hrd->initBufferFullness;
            mRenderHrd = true;

            break;
        }

        case VideoParamsTypeStoreMetaDataInBuffers: {
            VideoParamsStoreMetaDataInBuffers *metadata =
                    reinterpret_cast <VideoParamsStoreMetaDataInBuffers *> (videoEncParams);

            if (metadata->size != sizeof (VideoParamsStoreMetaDataInBuffers)) {
                return ENCODE_INVALID_PARAMS;
            }

            mStoreMetaDataInBuffers.isEnabled = metadata->isEnabled;

            break;
        }

        case VideoParamsTypeTemporalLayer:{
            VideoParamsTemporalLayer *temporallayer =
                    reinterpret_cast <VideoParamsTemporalLayer *> (videoEncParams);

            if (temporallayer->size != sizeof(VideoParamsTemporalLayer)) {
                 return ENCODE_INVALID_PARAMS;
            }

            mComParams.numberOfLayer = temporallayer->numberOfLayer;
            mComParams.nPeriodicity = temporallayer->nPeriodicity;
            for(uint32_t i=0;i<temporallayer->nPeriodicity;i++)
                mComParams.nLayerID[i] = temporallayer->nLayerID[i];
            mRenderMultiTemporal = true;
            break;
        }

        case VideoParamsTypeAVC:
        case VideoParamsTypeH263:
        case VideoParamsTypeMP4:
        case VideoParamsTypeVC1:
        case VideoParamsTypeVP8: {
            ret = derivedSetParams(videoEncParams);
            break;
        }

        default: {
            LOG_E ("Wrong ParamType here\n");
            return ENCODE_INVALID_PARAMS;
        }
    }
    return ret;
}

Encode_Status VideoEncoderBase::getParameters(
        VideoParamConfigSet *videoEncParams) {

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(videoEncParams);
    LOG_V("Config type = %d\n", (int)videoEncParams->type);

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

        case VideoParamsTypeUpSteamBuffer: {

            // Get upstream buffer could happen
            // but not meaningful a lot
            break;
        }

        case VideoParamsTypeUsrptrBuffer: {
            VideoParamsUsrptrBuffer *usrptrBuffer =
                    reinterpret_cast <VideoParamsUsrptrBuffer *> (videoEncParams);

            if (usrptrBuffer->size != sizeof (VideoParamsUsrptrBuffer)) {
                return ENCODE_INVALID_PARAMS;
            }

            ret = getNewUsrptrFromSurface(
                    usrptrBuffer->width, usrptrBuffer->height, usrptrBuffer->format,
                    usrptrBuffer->expectedSize, &(usrptrBuffer->actualSize),
                    &(usrptrBuffer->stride), &(usrptrBuffer->usrPtr));

            break;
        }

        case VideoParamsTypeHRD: {
            VideoParamsHRD *hrd =
                    reinterpret_cast <VideoParamsHRD *> (videoEncParams);

            if (hrd->size != sizeof (VideoParamsHRD)) {
                return ENCODE_INVALID_PARAMS;
            }

            hrd->bufferSize = mHrdParam.bufferSize;
            hrd->initBufferFullness = mHrdParam.initBufferFullness;

            break;
        }

        case VideoParamsTypeStoreMetaDataInBuffers: {
            VideoParamsStoreMetaDataInBuffers *metadata =
                    reinterpret_cast <VideoParamsStoreMetaDataInBuffers *> (videoEncParams);

            if (metadata->size != sizeof (VideoParamsStoreMetaDataInBuffers)) {
                return ENCODE_INVALID_PARAMS;
            }

            metadata->isEnabled = mStoreMetaDataInBuffers.isEnabled;

            break;
        }

        case VideoParamsTypeProfileLevel: {
            VideoParamsProfileLevel *profilelevel =
                reinterpret_cast <VideoParamsProfileLevel *> (videoEncParams);

            if (profilelevel->size != sizeof (VideoParamsProfileLevel)) {
                return ENCODE_INVALID_PARAMS;
            }

            profilelevel->level = 0;
            if(queryProfileLevelConfig(mVADisplay, profilelevel->profile) == ENCODE_SUCCESS){
                profilelevel->isSupported = true;
                if(profilelevel->profile == VAProfileH264High)
                    profilelevel->level = 42;
                else if(profilelevel->profile == VAProfileH264Main)
                     profilelevel->level = 42;
                else if(profilelevel->profile == VAProfileH264Baseline)
                     profilelevel->level = 41;
                else{
                    profilelevel->level = 0;
                    profilelevel->isSupported = false;
                }
            }
        }

        case VideoParamsTypeTemporalLayer:{
            VideoParamsTemporalLayer *temporallayer =
                reinterpret_cast <VideoParamsTemporalLayer *> (videoEncParams);

            if(temporallayer->size != sizeof(VideoParamsTemporalLayer)) {
                return ENCODE_INVALID_PARAMS;
            }

            temporallayer->numberOfLayer = mComParams.numberOfLayer;

            break;
        }

        case VideoParamsTypeAVC:
        case VideoParamsTypeH263:
        case VideoParamsTypeMP4:
        case VideoParamsTypeVC1:
        case VideoParamsTypeVP8: {
            derivedGetParams(videoEncParams);
            break;
        }

        default: {
            LOG_E ("Wrong ParamType here\n");
            break;
        }

    }
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::setConfig(VideoParamConfigSet *videoEncConfig) {

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(videoEncConfig);
    LOG_V("Config type = %d\n", (int)videoEncConfig->type);

   // workaround
#if 0
    if (!mStarted) {
        LOG_E("Encoder has not initialized yet, can't call setConfig\n");
        return ENCODE_NOT_INIT;
    }
#endif

    switch (videoEncConfig->type) {
        case VideoConfigTypeFrameRate: {
            VideoConfigFrameRate *configFrameRate =
                    reinterpret_cast <VideoConfigFrameRate *> (videoEncConfig);

            if (configFrameRate->size != sizeof (VideoConfigFrameRate)) {
                return ENCODE_INVALID_PARAMS;
            }
            mComParams.frameRate = configFrameRate->frameRate;
            mRenderFrameRate = true;
            break;
        }

        case VideoConfigTypeBitRate: {
            VideoConfigBitRate *configBitRate =
                    reinterpret_cast <VideoConfigBitRate *> (videoEncConfig);

            if (configBitRate->size != sizeof (VideoConfigBitRate)) {
                return ENCODE_INVALID_PARAMS;
            }

            if(mComParams.numberOfLayer == 1)
            {
                mComParams.rcParams = configBitRate->rcParams;
                mRenderBitRate = true;
            }
            else
            {
                mTemporalLayerBitrateFramerate[configBitRate->rcParams.temporalID].nLayerID = configBitRate->rcParams.temporalID;
                mTemporalLayerBitrateFramerate[configBitRate->rcParams.temporalID].bitRate = configBitRate->rcParams.bitRate;
                mTemporalLayerBitrateFramerate[configBitRate->rcParams.temporalID].frameRate = configBitRate->rcParams.temporalFrameRate;
            }
            break;
        }

        case VideoConfigTypeResolution: {

            // Not Implemented
            break;
        }
        case VideoConfigTypeIntraRefreshType: {

            VideoConfigIntraRefreshType *configIntraRefreshType =
                    reinterpret_cast <VideoConfigIntraRefreshType *> (videoEncConfig);

            if (configIntraRefreshType->size != sizeof (VideoConfigIntraRefreshType)) {
                return ENCODE_INVALID_PARAMS;
            }
            mComParams.refreshType = configIntraRefreshType->refreshType;
            break;
        }

        case VideoConfigTypeCyclicFrameInterval: {
            VideoConfigCyclicFrameInterval *configCyclicFrameInterval =
                    reinterpret_cast <VideoConfigCyclicFrameInterval *> (videoEncConfig);
            if (configCyclicFrameInterval->size != sizeof (VideoConfigCyclicFrameInterval)) {
                return ENCODE_INVALID_PARAMS;
            }

            mComParams.cyclicFrameInterval = configCyclicFrameInterval->cyclicFrameInterval;
            break;
        }

        case VideoConfigTypeAIR: {

            VideoConfigAIR *configAIR = reinterpret_cast <VideoConfigAIR *> (videoEncConfig);

            if (configAIR->size != sizeof (VideoConfigAIR)) {
                return ENCODE_INVALID_PARAMS;
            }

            mComParams.airParams = configAIR->airParams;
            mRenderAIR = true;
            break;
        }
        case VideoConfigTypeCIR: {

            VideoConfigCIR *configCIR = reinterpret_cast <VideoConfigCIR *> (videoEncConfig);

            if (configCIR->size != sizeof (VideoConfigCIR)) {
                return ENCODE_INVALID_PARAMS;
            }

            mComParams.cirParams = configCIR->cirParams;
            mRenderCIR = true;
            break;
        }
        case VideoConfigTypeAVCIntraPeriod:
        case VideoConfigTypeNALSize:
        case VideoConfigTypeIDRRequest:
        case VideoConfigTypeSliceNum:
        case VideoConfigTypeVP8:
        case VideoConfigTypeVP8ReferenceFrame:
        case VideoConfigTypeVP8MaxFrameSizeRatio:{
            ret = derivedSetConfig(videoEncConfig);
            break;
        }
        default: {
            LOG_E ("Wrong Config Type here\n");
            break;
        }
    }
    return ret;
}

Encode_Status VideoEncoderBase::getConfig(VideoParamConfigSet *videoEncConfig) {

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(videoEncConfig);
    LOG_V("Config type = %d\n", (int)videoEncConfig->type);

    switch (videoEncConfig->type) {
        case VideoConfigTypeFrameRate: {
            VideoConfigFrameRate *configFrameRate =
                    reinterpret_cast <VideoConfigFrameRate *> (videoEncConfig);

            if (configFrameRate->size != sizeof (VideoConfigFrameRate)) {
                return ENCODE_INVALID_PARAMS;
            }

            configFrameRate->frameRate = mComParams.frameRate;
            break;
        }

        case VideoConfigTypeBitRate: {
            VideoConfigBitRate *configBitRate =
                    reinterpret_cast <VideoConfigBitRate *> (videoEncConfig);

            if (configBitRate->size != sizeof (VideoConfigBitRate)) {
                return ENCODE_INVALID_PARAMS;
            }
            configBitRate->rcParams = mComParams.rcParams;


            break;
        }
        case VideoConfigTypeResolution: {
            // Not Implemented
            break;
        }
        case VideoConfigTypeIntraRefreshType: {

            VideoConfigIntraRefreshType *configIntraRefreshType =
                    reinterpret_cast <VideoConfigIntraRefreshType *> (videoEncConfig);

            if (configIntraRefreshType->size != sizeof (VideoConfigIntraRefreshType)) {
                return ENCODE_INVALID_PARAMS;
            }
            configIntraRefreshType->refreshType = mComParams.refreshType;
            break;
        }

        case VideoConfigTypeCyclicFrameInterval: {
            VideoConfigCyclicFrameInterval *configCyclicFrameInterval =
                    reinterpret_cast <VideoConfigCyclicFrameInterval *> (videoEncConfig);
            if (configCyclicFrameInterval->size != sizeof (VideoConfigCyclicFrameInterval)) {
                return ENCODE_INVALID_PARAMS;
            }

            configCyclicFrameInterval->cyclicFrameInterval = mComParams.cyclicFrameInterval;
            break;
        }

        case VideoConfigTypeAIR: {

            VideoConfigAIR *configAIR = reinterpret_cast <VideoConfigAIR *> (videoEncConfig);

            if (configAIR->size != sizeof (VideoConfigAIR)) {
                return ENCODE_INVALID_PARAMS;
            }

            configAIR->airParams = mComParams.airParams;
            break;
        }
        case VideoConfigTypeCIR: {

            VideoConfigCIR *configCIR = reinterpret_cast <VideoConfigCIR *> (videoEncConfig);

            if (configCIR->size != sizeof (VideoConfigCIR)) {
                return ENCODE_INVALID_PARAMS;
            }

            configCIR->cirParams = mComParams.cirParams;
            break;
        }
        case VideoConfigTypeAVCIntraPeriod:
        case VideoConfigTypeNALSize:
        case VideoConfigTypeIDRRequest:
        case VideoConfigTypeSliceNum:
        case VideoConfigTypeVP8: {

            ret = derivedGetConfig(videoEncConfig);
            break;
        }
        default: {
            LOG_E ("Wrong ParamType here\n");
            break;
        }
    }
    return ret;
}

void VideoEncoderBase:: PrepareFrameInfo (EncodeTask* task) {
    if (mNewHeader) mFrameNum = 0;
    LOG_V( "mFrameNum = %d   ", mFrameNum);

    updateFrameInfo(task) ;
}

Encode_Status VideoEncoderBase:: updateFrameInfo (EncodeTask* task) {

    task->type = FTYPE_P;

    // determine the picture type
    if (mFrameNum == 0)
        task->type = FTYPE_I;
    if (mComParams.intraPeriod != 0 && ((mFrameNum % mComParams.intraPeriod) == 0))
        task->type = FTYPE_I;

    if (task->type == FTYPE_I)
        task->flag |= ENCODE_BUFFERFLAG_SYNCFRAME;

    return ENCODE_SUCCESS;
}

Encode_Status  VideoEncoderBase::getMaxOutSize (uint32_t *maxSize) {

    uint32_t size = mComParams.resolution.width * mComParams.resolution.height;

    if (maxSize == NULL) {
        LOG_E("maxSize == NULL\n");
        return ENCODE_NULL_PTR;
    }

    LOG_V( "Begin\n");

    if (mCodedBufSize > 0) {
        *maxSize = mCodedBufSize;
        LOG_V ("Already calculate the max encoded size, get the value directly");
        return ENCODE_SUCCESS;
    }

    // here, VP8 is different from AVC/H263
    if(mComParams.profile == VAProfileVP8Version0_3) // for VP8 encode
    {
        // According to VIED suggestions, in CBR mode, coded buffer should be the size of 3 bytes per luma pixel
        // in CBR_HRD mode, coded buffer size should be  5 * rc_buf_sz * rc_target_bitrate;
        // now we just hardcode mCodedBufSize as 2M to walk round coded buffer size issue;
        /*
        if(mComParams.rcMode == VA_RC_CBR) // CBR_HRD mode
            mCodedBufSize = 5 * mComParams.rcParams.bitRate * 6000;
        else // CBR mode
            mCodedBufSize = 3 * mComParams.resolution.width * mComParams.resolution.height;
        */
        mCodedBufSize = (2 * 1024 * 1024 + 31) & (~31);
    }
    else // for AVC/H263/MPEG4 encode
    {
        // base on the rate control mode to calculate the defaule encoded buffer size
        if (mComParams.rcMode == VA_RC_NONE) {
             mCodedBufSize = (size * 400) / (16 * 16);
             // set to value according to QP
        } else {
             mCodedBufSize = mComParams.rcParams.bitRate / 4;
        }

        mCodedBufSize = max (mCodedBufSize , (size * 400) / (16 * 16));

        // in case got a very large user input bit rate value
        mCodedBufSize = min(mCodedBufSize, (size * 1.5 * 8));
        mCodedBufSize =  (mCodedBufSize + 15) &(~15);
    }

    *maxSize = mCodedBufSize;
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::getNewUsrptrFromSurface(
    uint32_t width, uint32_t height, uint32_t format,
    uint32_t expectedSize, uint32_t *outsize, uint32_t *stride, uint8_t **usrptr) {

    Encode_Status ret = ENCODE_FAIL;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    VASurfaceID surface = VA_INVALID_SURFACE;
    VAImage image;
    uint32_t index = 0;

    LOG_V( "Begin\n");
    // If encode session has been configured, we can not request surface creation anymore
    if (mStarted) {
        LOG_E( "Already Initialized, can not request VA surface anymore\n");
        return ENCODE_WRONG_STATE;
    }
    if (width<=0 || height<=0 ||outsize == NULL ||stride == NULL || usrptr == NULL) {
        LOG_E("width<=0 || height<=0 || outsize == NULL || stride == NULL ||usrptr == NULL\n");
        return ENCODE_NULL_PTR;
    }

    // Current only NV12 is supported in VA API
    // Through format we can get known the number of planes
    if (format != STRING_TO_FOURCC("NV12")) {
        LOG_W ("Format is not supported\n");
        return ENCODE_NOT_SUPPORTED;
    }

    surface = CreateNewVASurface(mVADisplay, width, height);
    if (surface == VA_INVALID_SURFACE)
        return ENCODE_DRIVER_FAIL;

    vaStatus = vaDeriveImage(mVADisplay, surface, &image);
    CHECK_VA_STATUS_RETURN("vaDeriveImage");
    LOG_V( "vaDeriveImage Done\n");
    vaStatus = vaMapBuffer(mVADisplay, image.buf, (void **) usrptr);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    // make sure the physical page been allocated
    for (index = 0; index < image.data_size; index = index + 4096) {
        unsigned char tmp =  *(*usrptr + index);
        if (tmp == 0)
            *(*usrptr + index) = 0;
    }

    *outsize = image.data_size;
    *stride = image.pitches[0];

    LOG_V( "surface = 0x%08x\n",(uint32_t)surface);
    LOG_V("image->pitches[0] = %d\n", image.pitches[0]);
    LOG_V("image->pitches[1] = %d\n", image.pitches[1]);
    LOG_V("image->offsets[0] = %d\n", image.offsets[0]);
    LOG_V("image->offsets[1] = %d\n", image.offsets[1]);
    LOG_V("image->num_planes = %d\n", image.num_planes);
    LOG_V("image->width = %d\n", image.width);
    LOG_V("image->height = %d\n", image.height);
    LOG_V("data_size = %d\n", image.data_size);
    LOG_V("usrptr = 0x%p\n", *usrptr);

    vaStatus = vaUnmapBuffer(mVADisplay, image.buf);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");
    vaStatus = vaDestroyImage(mVADisplay, image.image_id);
    CHECK_VA_STATUS_RETURN("vaDestroyImage");

    if (*outsize < expectedSize) {
        LOG_E ("Allocated buffer size is small than the expected size, destroy the surface");
        LOG_I ("Allocated size is %d, expected size is %d\n", *outsize, expectedSize);
        vaStatus = vaDestroySurfaces(mVADisplay, &surface, 1);
        CHECK_VA_STATUS_RETURN("vaDestroySurfaces");
        return ENCODE_FAIL;
    }

    VASurfaceMap *map = new VASurfaceMap(mVADisplay, mSupportedSurfaceMemType);
    if (map == NULL) {
        LOG_E( "new VASurfaceMap failed\n");
        return ENCODE_NO_MEMORY;
    }

    map->setVASurface(surface);  //special case, vasuface is set, so nothing do in doMapping
//    map->setType(MetadataBufferTypeEncoder);
    map->setValue((intptr_t)*usrptr);
    ValueInfo vinfo;
    memset(&vinfo, 0, sizeof(ValueInfo));
    vinfo.mode = (MemMode)MEM_MODE_USRPTR;
    vinfo.handle = 0;
    vinfo.size = 0;
    vinfo.width = width;
    vinfo.height = height;
    vinfo.lumaStride = width;
    vinfo.chromStride = width;
    vinfo.format = VA_FOURCC_NV12;
    vinfo.s3dformat = 0xffffffff;
    map->setValueInfo(vinfo);
    map->doMapping();

    mSrcSurfaceMapList.push_back(map);

    ret = ENCODE_SUCCESS;

    return ret;
}

Encode_Status VideoEncoderBase::setUpstreamBuffer(VideoParamsUpstreamBuffer *upStreamBuffer) {

    Encode_Status status = ENCODE_SUCCESS;

    CHECK_NULL_RETURN_IFFAIL(upStreamBuffer);
    if (upStreamBuffer->bufCnt == 0) {
        LOG_E("bufCnt == 0\n");
        return ENCODE_FAIL;
    }

    for(unsigned int i=0; i < upStreamBuffer->bufCnt; i++) {
        if (findSurfaceMapByValue(upStreamBuffer->bufList[i]) != NULL)  //already mapped
            continue;

        //wrap upstream buffer into vaSurface
        VASurfaceMap *map = new VASurfaceMap(mVADisplay, mSupportedSurfaceMemType);

//        map->setType(MetadataBufferTypeUser);
        map->setValue(upStreamBuffer->bufList[i]);
        ValueInfo vinfo;
        memset(&vinfo, 0, sizeof(ValueInfo));
        vinfo.mode = (MemMode)upStreamBuffer->bufferMode;
        vinfo.handle = (intptr_t)upStreamBuffer->display;
        vinfo.size = 0;
        if (upStreamBuffer->bufAttrib) {
            vinfo.width = upStreamBuffer->bufAttrib->realWidth;
            vinfo.height = upStreamBuffer->bufAttrib->realHeight;
            vinfo.lumaStride = upStreamBuffer->bufAttrib->lumaStride;
            vinfo.chromStride = upStreamBuffer->bufAttrib->chromStride;
            vinfo.format = upStreamBuffer->bufAttrib->format;
        }
        vinfo.s3dformat = 0xFFFFFFFF;
        map->setValueInfo(vinfo);
        status = map->doMapping();

        if (status == ENCODE_SUCCESS)
            mSrcSurfaceMapList.push_back(map);
        else
           delete map;
    }

    return status;
}

Encode_Status VideoEncoderBase::manageSrcSurface(VideoEncRawBuffer *inBuffer, VASurfaceID *sid) {

    Encode_Status ret = ENCODE_SUCCESS;
    IntelMetadataBufferType type;
    intptr_t value;
    ValueInfo vinfo;
    ValueInfo *pvinfo = &vinfo;
    intptr_t *extravalues = NULL;
    unsigned int extravalues_count = 0;

    IntelMetadataBuffer imb;
    VASurfaceMap *map = NULL;

    memset(&vinfo, 0, sizeof(ValueInfo));
    if (mStoreMetaDataInBuffers.isEnabled) {
        //metadatabuffer mode
        LOG_V("in metadata mode, data=%p, size=%d\n", inBuffer->data, inBuffer->size);
        if (imb.UnSerialize(inBuffer->data, inBuffer->size) != IMB_SUCCESS) {
            //fail to parse buffer
            return ENCODE_NO_REQUEST_DATA;
        }

        imb.GetType(type);
        imb.GetValue(value);
    } else {
        //raw mode
        LOG_I("in raw mode, data=%p, size=%d\n", inBuffer->data, inBuffer->size);
        if (! inBuffer->data || inBuffer->size == 0) {
            return ENCODE_NULL_PTR;
        }

        type = IntelMetadataBufferTypeUser;
        value = (intptr_t)inBuffer->data;
    }

#ifdef INTEL_VIDEO_XPROC_SHARING
    uint32_t sflag = mSessionFlag;
    imb.GetSessionFlag(mSessionFlag);
    if (mSessionFlag != sflag) {
        //new sharing session, flush buffer sharing cache
        IntelMetadataBuffer::ClearContext(sflag, false);
        //flush surfacemap cache
        LOG_V( "Flush Src Surface Map\n");
        while(! mSrcSurfaceMapList.empty())
        {
            delete (*mSrcSurfaceMapList.begin());
            mSrcSurfaceMapList.erase(mSrcSurfaceMapList.begin());
        }
    }
#endif

    //find if mapped
    map = (VASurfaceMap*) findSurfaceMapByValue(value);

    if (map) {
        //has mapped, get surfaceID directly and do all necessary actions
        LOG_V("direct find surface %d from value %i\n", map->getVASurface(), value);
        *sid = map->getVASurface();
        map->doMapping();
        return ret;
    }

    //if no found from list, then try to map value with parameters
    LOG_V("not find surface from cache with value %i, start mapping if enough information\n", value);

    if (mStoreMetaDataInBuffers.isEnabled) {

        //if type is IntelMetadataBufferTypeGrallocSource, use default parameters since no ValueInfo
        if (type == IntelMetadataBufferTypeGrallocSource) {
            vinfo.mode = MEM_MODE_GFXHANDLE;
            vinfo.handle = 0;
            vinfo.size = 0;
            vinfo.width = mComParams.resolution.width;
            vinfo.height = mComParams.resolution.height;
            vinfo.lumaStride = mComParams.resolution.width;
            vinfo.chromStride = mComParams.resolution.width;
            vinfo.format = VA_FOURCC_NV12;
            vinfo.s3dformat = 0xFFFFFFFF;
        } else {
            //get all info mapping needs
            imb.GetValueInfo(pvinfo);
            imb.GetExtraValues(extravalues, extravalues_count);
        }

    } else {

        //raw mode
        vinfo.mode = MEM_MODE_MALLOC;
        vinfo.handle = 0;
        vinfo.size = inBuffer->size;
        vinfo.width = mComParams.resolution.width;
        vinfo.height = mComParams.resolution.height;
        vinfo.lumaStride = mComParams.resolution.width;
        vinfo.chromStride = mComParams.resolution.width;
        vinfo.format = VA_FOURCC_NV12;
        vinfo.s3dformat = 0xFFFFFFFF;
    }

    /*  Start mapping, if pvinfo is not NULL, then have enough info to map;
     *   if extravalues is not NULL, then need to do more times mapping
     */
    if (pvinfo){
        //map according info, and add to surfacemap list
        map = new VASurfaceMap(mVADisplay, mSupportedSurfaceMemType);
        map->setValue(value);
        map->setValueInfo(*pvinfo);
        map->setAction(mVASurfaceMappingAction);

        ret = map->doMapping();
        if (ret == ENCODE_SUCCESS) {
            LOG_V("surface mapping success, map value %i into surface %d\n", value, map->getVASurface());
            mSrcSurfaceMapList.push_back(map);
        } else {
            delete map;
            LOG_E("surface mapping failed, wrong info or meet serious error\n");
            return ret;
        }

        *sid = map->getVASurface();

    } else {
        //can't map due to no info
        LOG_E("surface mapping failed, missing information\n");
        return ENCODE_NO_REQUEST_DATA;
    }

    if (extravalues) {
        //map more using same ValueInfo
        for(unsigned int i=0; i<extravalues_count; i++) {
            map = new VASurfaceMap(mVADisplay, mSupportedSurfaceMemType);
            map->setValue(extravalues[i]);
            map->setValueInfo(vinfo);

            ret = map->doMapping();
            if (ret == ENCODE_SUCCESS) {
                LOG_V("surface mapping extravalue success, map value %i into surface %d\n", extravalues[i], map->getVASurface());
                mSrcSurfaceMapList.push_back(map);
            } else {
                delete map;
                map = NULL;
                LOG_E( "surface mapping extravalue failed, extravalue is %i\n", extravalues[i]);
            }
        }
    }

    return ret;
}

Encode_Status VideoEncoderBase::renderDynamicBitrate(EncodeTask* task) {
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    LOG_V( "Begin\n\n");
    // disable bits stuffing and skip frame apply to all rate control mode

    VAEncMiscParameterBuffer   *miscEncParamBuf;
    VAEncMiscParameterRateControl *bitrateControlParam;
    VABufferID miscParamBufferID;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncMiscParameterBufferType,
            sizeof (VAEncMiscParameterBuffer) + sizeof (VAEncMiscParameterRateControl),
            1, NULL,
            &miscParamBufferID);

    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaMapBuffer(mVADisplay, miscParamBufferID, (void **)&miscEncParamBuf);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    miscEncParamBuf->type = VAEncMiscParameterTypeRateControl;
    bitrateControlParam = (VAEncMiscParameterRateControl *)miscEncParamBuf->data;

    bitrateControlParam->bits_per_second = mComParams.rcParams.bitRate;
    bitrateControlParam->initial_qp = mComParams.rcParams.initQP;
    if(mComParams.rcParams.enableIntraFrameQPControl && (task->type == FTYPE_IDR || task->type == FTYPE_I)) {
        bitrateControlParam->min_qp = mComParams.rcParams.I_minQP;
        bitrateControlParam->max_qp = mComParams.rcParams.I_maxQP;
        mRenderBitRate = true;
        LOG_I("apply I min/max qp for IDR or I frame\n");
    } else {
        bitrateControlParam->min_qp = mComParams.rcParams.minQP;
        bitrateControlParam->max_qp = mComParams.rcParams.maxQP;
        mRenderBitRate = false;
        LOG_I("revert to original min/max qp after IDR or I frame\n");
    }
    bitrateControlParam->target_percentage = mComParams.rcParams.targetPercentage;
    bitrateControlParam->window_size = mComParams.rcParams.windowSize;
    bitrateControlParam->rc_flags.bits.disable_frame_skip = mComParams.rcParams.disableFrameSkip;
    bitrateControlParam->rc_flags.bits.disable_bit_stuffing = mComParams.rcParams.disableBitsStuffing;
    bitrateControlParam->basic_unit_size = 0;

    LOG_I("bits_per_second = %d\n", bitrateControlParam->bits_per_second);
    LOG_I("initial_qp = %d\n", bitrateControlParam->initial_qp);
    LOG_I("min_qp = %d\n", bitrateControlParam->min_qp);
    LOG_I("max_qp = %d\n", bitrateControlParam->max_qp);
    LOG_I("target_percentage = %d\n", bitrateControlParam->target_percentage);
    LOG_I("window_size = %d\n", bitrateControlParam->window_size);
    LOG_I("disable_frame_skip = %d\n", bitrateControlParam->rc_flags.bits.disable_frame_skip);
    LOG_I("disable_bit_stuffing = %d\n", bitrateControlParam->rc_flags.bits.disable_bit_stuffing);

    vaStatus = vaUnmapBuffer(mVADisplay, miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext,
            &miscParamBufferID, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    return ENCODE_SUCCESS;
}


Encode_Status VideoEncoderBase::renderDynamicFrameRate() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    if (mComParams.rcMode != RATE_CONTROL_VCM) {

        LOG_W("Not in VCM mode, but call SendDynamicFramerate\n");
        return ENCODE_SUCCESS;
    }

    VAEncMiscParameterBuffer   *miscEncParamBuf;
    VAEncMiscParameterFrameRate *frameRateParam;
    VABufferID miscParamBufferID;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncMiscParameterBufferType,
            sizeof(miscEncParamBuf) + sizeof(VAEncMiscParameterFrameRate),
            1, NULL, &miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaMapBuffer(mVADisplay, miscParamBufferID, (void **)&miscEncParamBuf);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    miscEncParamBuf->type = VAEncMiscParameterTypeFrameRate;
    frameRateParam = (VAEncMiscParameterFrameRate *)miscEncParamBuf->data;
    frameRateParam->framerate =
            (unsigned int) (mComParams.frameRate.frameRateNum + mComParams.frameRate.frameRateDenom/2)
            / mComParams.frameRate.frameRateDenom;

    vaStatus = vaUnmapBuffer(mVADisplay, miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &miscParamBufferID, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_I( "frame rate = %d\n", frameRateParam->framerate);
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::renderHrd() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    VAEncMiscParameterBuffer *miscEncParamBuf;
    VAEncMiscParameterHRD *hrdParam;
    VABufferID miscParamBufferID;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncMiscParameterBufferType,
            sizeof(miscEncParamBuf) + sizeof(VAEncMiscParameterHRD),
            1, NULL, &miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaMapBuffer(mVADisplay, miscParamBufferID, (void **)&miscEncParamBuf);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    miscEncParamBuf->type = VAEncMiscParameterTypeHRD;
    hrdParam = (VAEncMiscParameterHRD *)miscEncParamBuf->data;

    hrdParam->buffer_size = mHrdParam.bufferSize;
    hrdParam->initial_buffer_fullness = mHrdParam.initBufferFullness;

    vaStatus = vaUnmapBuffer(mVADisplay, miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &miscParamBufferID, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    return ENCODE_SUCCESS;
}

VASurfaceMap *VideoEncoderBase::findSurfaceMapByValue(intptr_t value) {
    android::List<VASurfaceMap *>::iterator node;

    for(node = mSrcSurfaceMapList.begin(); node !=  mSrcSurfaceMapList.end(); node++)
    {
        if ((*node)->getValue() == value)
            return *node;
        else
            continue;
    }

    return NULL;
}
