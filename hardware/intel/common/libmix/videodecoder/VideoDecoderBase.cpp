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

#include "VideoDecoderBase.h"
#include "VideoDecoderTrace.h"
#include <string.h>
#include <va/va_android.h>
#include <va/va_tpi.h>
#ifdef  __SSE4_1__
#include "use_util_sse4.h"
#endif

#define INVALID_PTS ((uint64_t)-1)
#define MAXIMUM_POC  0x7FFFFFFF
#define MINIMUM_POC  0x80000000
#define ANDROID_DISPLAY_HANDLE 0x18C34078

VideoDecoderBase::VideoDecoderBase(const char *mimeType, _vbp_parser_type type)
    : mInitialized(false),
      mLowDelay(false),
      mDisplay(NULL),
      mVADisplay(NULL),
      mVAContext(VA_INVALID_ID),
      mVAConfig(VA_INVALID_ID),
      mVAStarted(false),
      mCurrentPTS(INVALID_PTS),
      mAcquiredBuffer(NULL),
      mLastReference(NULL),
      mForwardReference(NULL),
      mDecodingFrame(false),
      mSizeChanged(false),
      mShowFrame(true),
      mOutputWindowSize(OUTPUT_WINDOW_SIZE),
      mRotationDegrees(0),
      mErrReportEnabled(false),
      mWiDiOn(false),
      mRawOutput(false),
      mManageReference(true),
      mOutputMethod(OUTPUT_BY_PCT),
      mNumSurfaces(0),
      mSurfaceBuffers(NULL),
      mOutputHead(NULL),
      mOutputTail(NULL),
      mSurfaces(NULL),
      mVASurfaceAttrib(NULL),
      mSurfaceUserPtr(NULL),
      mSurfaceAcquirePos(0),
      mNextOutputPOC(MINIMUM_POC),
      mParserType(type),
      mParserHandle(NULL),
      mSignalBufferSize(0) {

    memset(&mVideoFormatInfo, 0, sizeof(VideoFormatInfo));
    memset(&mConfigBuffer, 0, sizeof(mConfigBuffer));
    for (int i = 0; i < MAX_GRAPHIC_BUFFER_NUM; i++) {
         mSignalBufferPre[i] = NULL;
    }
    pthread_mutex_init(&mLock, NULL);
    mVideoFormatInfo.mimeType = strdup(mimeType);
    mUseGEN = false;
    mLibHandle = NULL;
    mParserOpen = NULL;
    mParserClose = NULL;
    mParserParse = NULL;
    mParserQuery = NULL;
    mParserFlush = NULL;
    mParserUpdate = NULL;
}

VideoDecoderBase::~VideoDecoderBase() {
    pthread_mutex_destroy(&mLock);
    stop();
    free(mVideoFormatInfo.mimeType);
}

Decode_Status VideoDecoderBase::start(VideoConfigBuffer *buffer) {
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }

    if (mParserHandle != NULL) {
        WTRACE("Decoder has already started.");
        return DECODE_SUCCESS;
    }
    mLibHandle = dlopen("libmixvbp.so", RTLD_NOW);
    if (mLibHandle == NULL) {
       return DECODE_NO_PARSER;
    }
    mParserOpen = (OpenFunc)dlsym(mLibHandle, "vbp_open");
    mParserClose = (CloseFunc)dlsym(mLibHandle, "vbp_close");
    mParserParse = (ParseFunc)dlsym(mLibHandle, "vbp_parse");
    mParserQuery = (QueryFunc)dlsym(mLibHandle, "vbp_query");
    mParserFlush = (FlushFunc)dlsym(mLibHandle, "vbp_flush");
    if (mParserOpen == NULL || mParserClose == NULL || mParserParse == NULL
        || mParserQuery == NULL || mParserFlush == NULL) {
        return DECODE_NO_PARSER;
    }
#if (defined USE_AVC_SHORT_FORMAT || defined USE_SLICE_HEADER_PARSING)
    mParserUpdate = (UpdateFunc)dlsym(mLibHandle, "vbp_update");
    if (mParserUpdate == NULL) {
        return DECODE_NO_PARSER;
    }
#endif
    if ((int32_t)mParserType != VBP_INVALID) {
        ITRACE("mParserType = %d", mParserType);
        if (mParserOpen(mParserType, &mParserHandle) != VBP_OK) {
            ETRACE("Failed to open VBP parser.");
            return DECODE_NO_PARSER;
        }
    }
    // keep a copy of configure buffer, meta data only. It can be used to override VA setup parameter.
    mConfigBuffer = *buffer;
    mConfigBuffer.data = NULL;
    mConfigBuffer.size = 0;

    mVideoFormatInfo.width = buffer->width;
    mVideoFormatInfo.height = buffer->height;
    if (buffer->flag & USE_NATIVE_GRAPHIC_BUFFER) {
        mVideoFormatInfo.surfaceWidth = buffer->graphicBufferWidth;
        mVideoFormatInfo.surfaceHeight = buffer->graphicBufferHeight;
    }
    mLowDelay = buffer->flag & WANT_LOW_DELAY;
    mRawOutput = buffer->flag & WANT_RAW_OUTPUT;
    if (mRawOutput) {
        WTRACE("Output is raw data.");
    }

    return DECODE_SUCCESS;
}


Decode_Status VideoDecoderBase::reset(VideoConfigBuffer *buffer) {
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }

    // if VA is already started, terminate VA as graphic buffers are reallocated by omxcodec
    terminateVA();

    // reset the mconfigBuffer to pass it for startVA.
    mConfigBuffer = *buffer;
    mConfigBuffer.data = NULL;
    mConfigBuffer.size = 0;

    mVideoFormatInfo.width = buffer->width;
    mVideoFormatInfo.height = buffer->height;
    if (buffer->flag & USE_NATIVE_GRAPHIC_BUFFER) {
        mVideoFormatInfo.surfaceWidth = buffer->graphicBufferWidth;
        mVideoFormatInfo.surfaceHeight = buffer->graphicBufferHeight;
    }
    mVideoFormatInfo.actualBufferNeeded = mConfigBuffer.surfaceNumber;
    mLowDelay = buffer->flag & WANT_LOW_DELAY;
    mRawOutput = buffer->flag & WANT_RAW_OUTPUT;
    if (mRawOutput) {
        WTRACE("Output is raw data.");
    }
    return DECODE_SUCCESS;
}



void VideoDecoderBase::stop(void) {
    terminateVA();

    mCurrentPTS = INVALID_PTS;
    mAcquiredBuffer = NULL;
    mLastReference = NULL;
    mForwardReference = NULL;
    mDecodingFrame = false;
    mSizeChanged = false;

    // private variables
    mLowDelay = false;
    mRawOutput = false;
    mNumSurfaces = 0;
    mSurfaceAcquirePos = 0;
    mNextOutputPOC = MINIMUM_POC;
    mVideoFormatInfo.valid = false;
    if (mParserHandle){
        mParserClose(mParserHandle);
        mParserHandle = NULL;
    }
    if (mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

void VideoDecoderBase::flush(void) {
    if (mVAStarted == false) {
        // nothing to flush at this stage
        return;
    }

    endDecodingFrame(true);

    VideoSurfaceBuffer *p = mOutputHead;
    // check if there's buffer with DRC flag in the output queue
    while (p) {
        if (p->renderBuffer.flag & IS_RESOLUTION_CHANGE) {
            mSizeChanged = true;
            break;
        }
        p = p->next;
    }
    // avoid setting mSurfaceAcquirePos  to 0 as it may cause tearing
    // (surface is still being rendered)
    mSurfaceAcquirePos = (mSurfaceAcquirePos  + 1) % mNumSurfaces;
    mNextOutputPOC = MINIMUM_POC;
    mCurrentPTS = INVALID_PTS;
    mAcquiredBuffer = NULL;
    mLastReference = NULL;
    mForwardReference = NULL;
    mOutputHead = NULL;
    mOutputTail = NULL;
    mDecodingFrame = false;

    // flush vbp parser
    if (mParserHandle && (mParserFlush(mParserHandle) != VBP_OK)) {
        WTRACE("Failed to flush parser. Continue");
    }

    // initialize surface buffer without resetting mapped/raw data
    initSurfaceBuffer(false);

}

void VideoDecoderBase::freeSurfaceBuffers(void) {
    if (mVAStarted == false) {
        // nothing to free surface buffers at this stage
        return;
    }

    pthread_mutex_lock(&mLock);

    endDecodingFrame(true);

    // if VA is already started, terminate VA as graphic buffers are reallocated by omxcodec
    terminateVA();

    pthread_mutex_unlock(&mLock);
}

const VideoFormatInfo* VideoDecoderBase::getFormatInfo(void) {
    return &mVideoFormatInfo;
}

const VideoRenderBuffer* VideoDecoderBase::getOutput(bool draining, VideoErrorBuffer *outErrBuf) {
    VAStatus vaStatus;
    if (mVAStarted == false) {
        return NULL;
    }
    bool useGraphicBuffer = mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER;

    if (draining) {
        // complete decoding the last frame and ignore return
        endDecodingFrame(false);
    }

    if (mOutputHead == NULL) {
        return NULL;
    }

    // output by position (the first buffer)
    VideoSurfaceBuffer *outputByPos = mOutputHead;

    if (mLowDelay) {
        mOutputHead = mOutputHead->next;
        if (mOutputHead == NULL) {
            mOutputTail = NULL;
        }
        vaStatus = vaSetTimestampForSurface(mVADisplay, outputByPos->renderBuffer.surface, outputByPos->renderBuffer.timeStamp);
        if (useGraphicBuffer && !mUseGEN) {
            vaSyncSurface(mVADisplay, outputByPos->renderBuffer.surface);
            fillDecodingErrors(&(outputByPos->renderBuffer));
        }
        if (draining && mOutputTail == NULL) {
            outputByPos->renderBuffer.flag |= IS_EOS;
        }
        drainDecodingErrors(outErrBuf, &(outputByPos->renderBuffer));

        return &(outputByPos->renderBuffer);
    }

    VideoSurfaceBuffer *output = NULL;
    if (mOutputMethod == OUTPUT_BY_POC) {
        output = findOutputByPoc(draining);
    } else if (mOutputMethod == OUTPUT_BY_PCT) {
        output = findOutputByPct(draining);
    } else {
        ETRACE("Invalid output method.");
        return NULL;
    }

    if (output == NULL) {
        return NULL;
    }

    if (output != outputByPos) {
        // remove this output from middle or end of the list
        VideoSurfaceBuffer *p = outputByPos;
        while (p->next != output) {
            p = p->next;
        }
        p->next = output->next;
        if (mOutputTail == output) {
            mOutputTail = p;
        }
    } else {
        // remove this output from head of the list
        mOutputHead = mOutputHead->next;
        if (mOutputHead == NULL) {
            mOutputTail = NULL;
        }
    }
    //VTRACE("Output POC %d for display (pts = %.2f)", output->pictureOrder, output->renderBuffer.timeStamp/1E6);
    vaStatus = vaSetTimestampForSurface(mVADisplay, output->renderBuffer.surface, output->renderBuffer.timeStamp);

    if (useGraphicBuffer && !mUseGEN) {
        vaSyncSurface(mVADisplay, output->renderBuffer.surface);
        fillDecodingErrors(&(output->renderBuffer));
    }

    if (draining && mOutputTail == NULL) {
        output->renderBuffer.flag |= IS_EOS;
    }

    drainDecodingErrors(outErrBuf, &(output->renderBuffer));

    return &(output->renderBuffer);
}

VideoSurfaceBuffer* VideoDecoderBase::findOutputByPts() {
    // output by presentation time stamp - buffer with the smallest time stamp is output
    VideoSurfaceBuffer *p = mOutputHead;
    VideoSurfaceBuffer *outputByPts = NULL;
    uint64_t pts = INVALID_PTS;
    do {
        if ((uint64_t)(p->renderBuffer.timeStamp) <= pts) {
            // find buffer with the smallest PTS
            pts = p->renderBuffer.timeStamp;
            outputByPts = p;
        }
        p = p->next;
    } while (p != NULL);

    return outputByPts;
}

VideoSurfaceBuffer* VideoDecoderBase::findOutputByPct(bool draining) {
    // output by picture coding type (PCT)
    // if there is more than one reference frame, the first reference frame is ouput, otherwise,
    // output non-reference frame if there is any.

    VideoSurfaceBuffer *p = mOutputHead;
    VideoSurfaceBuffer *outputByPct = NULL;
    int32_t reference = 0;
    do {
        if (p->referenceFrame) {
            reference++;
            if (reference > 1) {
                // mOutputHead must be a reference frame
                outputByPct = mOutputHead;
                break;
            }
        } else {
            // first non-reference frame
            outputByPct = p;
            break;
        }
        p = p->next;
    } while (p != NULL);

    if (outputByPct == NULL && draining) {
        outputByPct = mOutputHead;
    }
    return  outputByPct;
}

#if 0
VideoSurfaceBuffer* VideoDecoderBase::findOutputByPoc(bool draining) {
    // output by picture order count (POC)
    // Output criteria:
    // if there is IDR frame (POC == 0), all the frames before IDR must be output;
    // Otherwise, if draining flag is set or list is full, frame with the least POC is output;
    // Otherwise, NOTHING is output

    int32_t dpbFullness = 0;
    for (int32_t i = 0; i < mNumSurfaces; i++) {
        // count num of reference frames
        if (mSurfaceBuffers[i].asReferernce) {
            dpbFullness++;
        }
    }

    if (mAcquiredBuffer && mAcquiredBuffer->asReferernce) {
        // frame is being decoded and is not ready for output yet
        dpbFullness--;
    }

    VideoSurfaceBuffer *p = mOutputHead;
    while (p != NULL) {
        // count dpbFullness with non-reference frame in the output queue
        if (p->asReferernce == false) {
            dpbFullness++;
        }
        p = p->next;
    }

Retry:
    p = mOutputHead;
    VideoSurfaceBuffer *outputByPoc = NULL;
    int32_t count = 0;
    int32_t poc = MAXIMUM_POC;

    do {
        if (p->pictureOrder == 0) {
            // output picture with the least POC before IDR
            if (outputByPoc != NULL) {
                mNextOutputPOC = outputByPoc->pictureOrder + 1;
                return outputByPoc;
            } else {
                mNextOutputPOC = MINIMUM_POC;
            }
        }

        // POC of  the output candidate must not be less than mNextOutputPOC
        if (p->pictureOrder < mNextOutputPOC) {
            break;
        }

        if (p->pictureOrder < poc) {
            // update the least POC.
            poc = p->pictureOrder;
            outputByPoc = p;
        }
        count++;
        p = p->next;
    } while (p != NULL && count < mOutputWindowSize);

    if (draining == false && dpbFullness < mOutputWindowSize) {
        // list is not  full and we are not  in draining state
        // if DPB is already full, one frame must be output
        return NULL;
    }

    if (outputByPoc == NULL) {
        mNextOutputPOC = MINIMUM_POC;
        goto Retry;
    }

    // for debugging purpose
    if (outputByPoc->pictureOrder != 0 && outputByPoc->pictureOrder < mNextOutputPOC) {
        ETRACE("Output POC is not incremental, expected %d, actual %d", mNextOutputPOC, outputByPoc->pictureOrder);
        //gaps_in_frame_num_value_allowed_flag is not currently supported
    }

    mNextOutputPOC = outputByPoc->pictureOrder + 1;

    return outputByPoc;
}
#else
VideoSurfaceBuffer* VideoDecoderBase::findOutputByPoc(bool draining) {
    VideoSurfaceBuffer *output = NULL;
    VideoSurfaceBuffer *p = mOutputHead;
    int32_t count = 0;
    int32_t poc = MAXIMUM_POC;
    VideoSurfaceBuffer *outputleastpoc = mOutputHead;
    do {
        count++;
        if (p->pictureOrder == 0) {
            // any picture before this POC (new IDR) must be output
            if (output == NULL) {
                mNextOutputPOC = MINIMUM_POC;
                // looking for any POC with negative value
            } else {
                mNextOutputPOC = output->pictureOrder + 1;
                break;
            }
        }
        if (p->pictureOrder < poc && p->pictureOrder >= mNextOutputPOC) {
            // this POC meets ouput criteria.
            poc = p->pictureOrder;
            output = p;
            outputleastpoc = p;
        }
        if (poc == mNextOutputPOC || count == mOutputWindowSize) {
            if (output != NULL) {
                // this indicates two cases:
                // 1) the next output POC is found.
                // 2) output queue is full and there is at least one buffer meeting the output criteria.
                mNextOutputPOC = output->pictureOrder + 1;
                break;
            } else {
                // this indicates output queue is full and no buffer in the queue meets the output criteria
                // restart processing as queue is FULL and output criteria is changed. (next output POC is 0)
                mNextOutputPOC = MINIMUM_POC;
                count = 0;
                poc = MAXIMUM_POC;
                p = mOutputHead;
                continue;
            }
        }
        if (p->next == NULL) {
            output = NULL;
        }

        p = p->next;
    } while (p != NULL);

    if (draining == true && output == NULL) {
        output = outputleastpoc;
    }

    return output;
}
#endif

bool VideoDecoderBase::checkBufferAvail(void) {
    if (!mInitialized) {
        if ((mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER) == 0) {
            return true;
        }
        for (int i = 0; i < MAX_GRAPHIC_BUFFER_NUM; i++) {
            if (mSignalBufferPre[i] != NULL) {
                return true;
            }
        }
        return false;
    }
    // check whether there is buffer available for decoding
    // TODO: check frame being referenced for frame skipping
    VideoSurfaceBuffer *buffer = NULL;
    for (int32_t i = 0; i < mNumSurfaces; i++) {
        buffer = mSurfaceBuffers + i;

        if (buffer->asReferernce == false &&
            buffer->renderBuffer.renderDone == true) {
            querySurfaceRenderStatus(buffer);
            if (buffer->renderBuffer.driverRenderDone == true)
                return true;
        }
     }
    return false;
}

Decode_Status VideoDecoderBase::acquireSurfaceBuffer(void) {
    if (mVAStarted == false) {
        return DECODE_FAIL;
    }

    if (mAcquiredBuffer != NULL) {
        ETRACE("mAcquiredBuffer is not NULL. Implementation bug.");
        return DECODE_FAIL;
    }

    int nextAcquire = mSurfaceAcquirePos;
    VideoSurfaceBuffer *acquiredBuffer = NULL;
    bool acquired = false;

    while (acquired == false) {
        acquiredBuffer = mSurfaceBuffers + nextAcquire;

        querySurfaceRenderStatus(acquiredBuffer);

        if (acquiredBuffer->asReferernce == false && acquiredBuffer->renderBuffer.renderDone == true && acquiredBuffer->renderBuffer.driverRenderDone == true) {
            // this is potential buffer for acquisition. Check if it is referenced by other surface for frame skipping
            VideoSurfaceBuffer *temp;
            acquired = true;
            for (int i = 0; i < mNumSurfaces; i++) {
                if (i == nextAcquire) {
                    continue;
                }
                temp = mSurfaceBuffers + i;
                // use mSurfaces[nextAcquire] instead of acquiredBuffer->renderBuffer.surface as its the actual surface to use.
                if (temp->renderBuffer.surface == mSurfaces[nextAcquire] &&
                    temp->renderBuffer.renderDone == false) {
                    ITRACE("Surface is referenced by other surface buffer.");
                    acquired = false;
                    break;
                }
            }
        }
        if (acquired) {
            break;
        }
        nextAcquire++;
        if (nextAcquire == mNumSurfaces) {
            nextAcquire = 0;
        }
        if (nextAcquire == mSurfaceAcquirePos) {
            return DECODE_NO_SURFACE;
        }
    }

    if (acquired == false) {
        return DECODE_NO_SURFACE;
    }

    mAcquiredBuffer = acquiredBuffer;
    mSurfaceAcquirePos = nextAcquire;

    // set surface again as surface maybe reset by skipped frame.
    // skipped frame is a "non-coded frame" and decoder needs to duplicate the previous reference frame as the output.
    mAcquiredBuffer->renderBuffer.surface = mSurfaces[mSurfaceAcquirePos];
    if (mSurfaceUserPtr && mAcquiredBuffer->mappedData) {
        mAcquiredBuffer->mappedData->data = mSurfaceUserPtr[mSurfaceAcquirePos];
    }
    mAcquiredBuffer->renderBuffer.timeStamp = INVALID_PTS;
    mAcquiredBuffer->renderBuffer.display = mVADisplay;
    mAcquiredBuffer->renderBuffer.flag = 0;
    mAcquiredBuffer->renderBuffer.renderDone = false;
    mAcquiredBuffer->asReferernce = false;
    mAcquiredBuffer->renderBuffer.errBuf.errorNumber = 0;
    mAcquiredBuffer->renderBuffer.errBuf.timeStamp = INVALID_PTS;

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::outputSurfaceBuffer(void) {
    Decode_Status status;
    if (mAcquiredBuffer == NULL) {
        ETRACE("mAcquiredBuffer is NULL. Implementation bug.");
        return DECODE_FAIL;
    }

    if (mRawOutput) {
        status = getRawDataFromSurface();
        CHECK_STATUS();
    }

    // frame is successfly decoded to the current surface,  it is ready for output
    if (mShowFrame) {
        mAcquiredBuffer->renderBuffer.renderDone = false;
    } else {
        mAcquiredBuffer->renderBuffer.renderDone = true;
    }

    // decoder must set "asReference and referenceFrame" flags properly

    // update reference frames
    if (mAcquiredBuffer->referenceFrame) {
        if (mManageReference) {
            // managing reference for MPEG4/H.263/WMV.
            // AVC should manage reference frame in a different way
            if (mForwardReference != NULL) {
                // this foward reference is no longer needed
                mForwardReference->asReferernce = false;
            }
            // Forware reference for either P or B frame prediction
            mForwardReference = mLastReference;
            mAcquiredBuffer->asReferernce = true;
        }

        // the last reference frame.
        mLastReference = mAcquiredBuffer;
    }
    // add to the output list
    if (mShowFrame) {
        if (mOutputHead == NULL) {
            mOutputHead = mAcquiredBuffer;
        } else {
            mOutputTail->next = mAcquiredBuffer;
        }
        mOutputTail = mAcquiredBuffer;
        mOutputTail->next = NULL;
    }

    //VTRACE("Pushing POC %d to queue (pts = %.2f)", mAcquiredBuffer->pictureOrder, mAcquiredBuffer->renderBuffer.timeStamp/1E6);

    mAcquiredBuffer = NULL;
    mSurfaceAcquirePos = (mSurfaceAcquirePos  + 1 ) % mNumSurfaces;
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::releaseSurfaceBuffer(void) {
    if (mAcquiredBuffer == NULL) {
        // this is harmless error
        return DECODE_SUCCESS;
    }

    // frame is not decoded to the acquired buffer, current surface is invalid, and can't be output.
    mAcquiredBuffer->asReferernce = false;
    mAcquiredBuffer->renderBuffer.renderDone = true;
    mAcquiredBuffer = NULL;
    return DECODE_SUCCESS;
}

void VideoDecoderBase::flushSurfaceBuffers(void) {
    endDecodingFrame(true);
    VideoSurfaceBuffer *p = NULL;
    while (mOutputHead) {
        mOutputHead->renderBuffer.renderDone = true;
        p = mOutputHead;
        mOutputHead = mOutputHead->next;
        p->next = NULL;
    }
    mOutputHead = NULL;
    mOutputTail = NULL;
}

Decode_Status VideoDecoderBase::endDecodingFrame(bool dropFrame) {
    Decode_Status status = DECODE_SUCCESS;
    VAStatus vaStatus;

    if (mDecodingFrame == false) {
        if (mAcquiredBuffer != NULL) {
            //ETRACE("mAcquiredBuffer is not NULL. Implementation bug.");
            releaseSurfaceBuffer();
            status = DECODE_FAIL;
        }
        return status;
    }
    // return through exit label to reset mDecodingFrame
    if (mAcquiredBuffer == NULL) {
        ETRACE("mAcquiredBuffer is NULL. Implementation bug.");
        status = DECODE_FAIL;
        goto exit;
    }

    vaStatus = vaEndPicture(mVADisplay, mVAContext);
    if (vaStatus != VA_STATUS_SUCCESS) {
        releaseSurfaceBuffer();
        ETRACE("vaEndPicture failed. vaStatus = %d", vaStatus);
        status = DECODE_DRIVER_FAIL;
        goto exit;
    }

    if (dropFrame) {
        // we are asked to drop this decoded picture
        VTRACE("Frame dropped in endDecodingFrame");
        vaStatus = vaSyncSurface(mVADisplay, mAcquiredBuffer->renderBuffer.surface);
        releaseSurfaceBuffer();
        goto exit;
    }
    status = outputSurfaceBuffer();
    // fall through
exit:
    mDecodingFrame = false;
    return status;
}


Decode_Status VideoDecoderBase::setupVA(uint32_t numSurface, VAProfile profile, uint32_t numExtraSurface) {
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    Decode_Status status;
    VAConfigAttrib attrib;

    if (mVAStarted) {
        return DECODE_SUCCESS;
    }

    mRotationDegrees = 0;
    if (mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER){
#ifdef TARGET_HAS_ISV
        if (mVideoFormatInfo.actualBufferNeeded > mConfigBuffer.surfaceNumber - mConfigBuffer.vppBufferNum)
#else
        if (mVideoFormatInfo.actualBufferNeeded > mConfigBuffer.surfaceNumber)
#endif
            return DECODE_FORMAT_CHANGE;

        numSurface = mConfigBuffer.surfaceNumber;
        // if format has been changed in USE_NATIVE_GRAPHIC_BUFFER mode,
        // we can not setupVA here when the graphic buffer resolution is smaller than the resolution decoder really needs
        if (mSizeChanged) {
            if (mVideoFormatInfo.surfaceWidth < mVideoFormatInfo.width || mVideoFormatInfo.surfaceHeight < mVideoFormatInfo.height) {
                mSizeChanged = false;
                return DECODE_FORMAT_CHANGE;
            }
        }
    }

    // TODO: validate profile
    if (numSurface == 0) {
        return DECODE_FAIL;
    }

    if (mConfigBuffer.flag & HAS_MINIMUM_SURFACE_NUMBER) {
        if (numSurface < mConfigBuffer.surfaceNumber) {
            WTRACE("surface to allocated %d is less than minimum number required %d",
                    numSurface, mConfigBuffer.surfaceNumber);
            numSurface = mConfigBuffer.surfaceNumber;
        }
    }

    if (mVADisplay != NULL) {
        ETRACE("VA is partially started.");
        return DECODE_FAIL;
    }

    // Display is defined as "unsigned int"
#ifndef USE_HYBRID_DRIVER
    mDisplay = new Display;
    *mDisplay = ANDROID_DISPLAY_HANDLE;
#else
    if (profile >= VAProfileH264Baseline && profile <= VAProfileVC1Advanced) {
        ITRACE("Using GEN driver");
        mDisplay = "libva_driver_name=i965";
        mUseGEN = true;
    } else {
        ITRACE("Using PVR driver");
        mDisplay = "libva_driver_name=pvr";
        mUseGEN = false;
    }

#endif
    mVADisplay = vaGetDisplay(mDisplay);
    if (mVADisplay == NULL) {
        ETRACE("vaGetDisplay failed.");
        return DECODE_DRIVER_FAIL;
    }

    int majorVersion, minorVersion;
    vaStatus = vaInitialize(mVADisplay, &majorVersion, &minorVersion);
    CHECK_VA_STATUS("vaInitialize");

    if ((int32_t)profile != VAProfileSoftwareDecoding) {

        status = checkHardwareCapability();
        CHECK_STATUS("checkHardwareCapability");

#if (defined USE_AVC_SHORT_FORMAT || defined USE_SLICE_HEADER_PARSING)
        status = getCodecSpecificConfigs(profile, &mVAConfig);
        CHECK_STATUS("getCodecSpecificAttributes");
#else
        //We are requesting RT attributes
        attrib.type = VAConfigAttribRTFormat;
        attrib.value = VA_RT_FORMAT_YUV420;

        vaStatus = vaCreateConfig(
                mVADisplay,
                profile,
                VAEntrypointVLD,
                &attrib,
                1,
                &mVAConfig);
        CHECK_VA_STATUS("vaCreateConfig");
#endif
    }

    mNumSurfaces = numSurface;
    mNumExtraSurfaces = numExtraSurface;
    mSurfaces = new VASurfaceID [mNumSurfaces + mNumExtraSurfaces];
    mExtraSurfaces = mSurfaces + mNumSurfaces;
    if (mSurfaces == NULL) {
        return DECODE_MEMORY_FAIL;
    }

    setRenderRect();

    int32_t format = VA_RT_FORMAT_YUV420;
    if (mConfigBuffer.flag & WANT_SURFACE_PROTECTION) {
#ifndef USE_AVC_SHORT_FORMAT
        format |= VA_RT_FORMAT_PROTECTED;
        WTRACE("Surface is protected.");
#endif
    }
    if (mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER) {
        VASurfaceAttrib attribs[2];
        mVASurfaceAttrib = new VASurfaceAttribExternalBuffers;
        if (mVASurfaceAttrib == NULL) {
            return DECODE_MEMORY_FAIL;
        }

        mVASurfaceAttrib->buffers= (unsigned long *)malloc(sizeof(unsigned long)*mNumSurfaces);
        if (mVASurfaceAttrib->buffers == NULL) {
            return DECODE_MEMORY_FAIL;
        }
        mVASurfaceAttrib->num_buffers = mNumSurfaces;
        mVASurfaceAttrib->pixel_format = VA_FOURCC_NV12;
        mVASurfaceAttrib->width = mVideoFormatInfo.surfaceWidth;
        mVASurfaceAttrib->height = mVideoFormatInfo.surfaceHeight;
        mVASurfaceAttrib->data_size = mConfigBuffer.graphicBufferStride * mVideoFormatInfo.surfaceHeight * 1.5;
        mVASurfaceAttrib->num_planes = 2;
        mVASurfaceAttrib->pitches[0] = mConfigBuffer.graphicBufferStride;
        mVASurfaceAttrib->pitches[1] = mConfigBuffer.graphicBufferStride;
        mVASurfaceAttrib->pitches[2] = 0;
        mVASurfaceAttrib->pitches[3] = 0;
        mVASurfaceAttrib->offsets[0] = 0;
        mVASurfaceAttrib->offsets[1] = mConfigBuffer.graphicBufferStride * mVideoFormatInfo.surfaceHeight;
        mVASurfaceAttrib->offsets[2] = 0;
        mVASurfaceAttrib->offsets[3] = 0;
        mVASurfaceAttrib->private_data = (void *)mConfigBuffer.nativeWindow;
        mVASurfaceAttrib->flags = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;
        if (mConfigBuffer.flag & USE_TILING_MEMORY)
            mVASurfaceAttrib->flags |= VA_SURFACE_EXTBUF_DESC_ENABLE_TILING;

        for (int i = 0; i < mNumSurfaces; i++) {
            mVASurfaceAttrib->buffers[i] = (unsigned long)mConfigBuffer.graphicBufferHandler[i];
        }

        attribs[0].type = (VASurfaceAttribType)VASurfaceAttribMemoryType;
        attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
        attribs[0].value.type = VAGenericValueTypeInteger;
        attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;

        attribs[1].type = (VASurfaceAttribType)VASurfaceAttribExternalBufferDescriptor;
        attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
        attribs[1].value.type = VAGenericValueTypePointer;
        attribs[1].value.value.p = (void *)mVASurfaceAttrib;

        vaStatus = vaCreateSurfaces(
            mVADisplay,
            format,
            mVideoFormatInfo.surfaceWidth,
            mVideoFormatInfo.surfaceHeight,
            mSurfaces,
            mNumSurfaces,
            attribs,
            2);

    } else {
        vaStatus = vaCreateSurfaces(
            mVADisplay,
            format,
            mVideoFormatInfo.width,
            mVideoFormatInfo.height,
            mSurfaces,
            mNumSurfaces,
            NULL,
            0);
        mVideoFormatInfo.surfaceWidth = mVideoFormatInfo.width;
        mVideoFormatInfo.surfaceHeight = mVideoFormatInfo.height;
    }
    CHECK_VA_STATUS("vaCreateSurfaces");

    if (mNumExtraSurfaces != 0) {
        vaStatus = vaCreateSurfaces(
            mVADisplay,
            format,
            mVideoFormatInfo.surfaceWidth,
            mVideoFormatInfo.surfaceHeight,
            mExtraSurfaces,
            mNumExtraSurfaces,
            NULL,
            0);
        CHECK_VA_STATUS("vaCreateSurfaces");
    }

    mVideoFormatInfo.surfaceNumber = mNumSurfaces;
    mVideoFormatInfo.ctxSurfaces = mSurfaces;

    if ((int32_t)profile != VAProfileSoftwareDecoding) {
        vaStatus = vaCreateContext(
                mVADisplay,
                mVAConfig,
                mVideoFormatInfo.surfaceWidth,
                mVideoFormatInfo.surfaceHeight,
                0,
                mSurfaces,
                mNumSurfaces + mNumExtraSurfaces,
                &mVAContext);
        CHECK_VA_STATUS("vaCreateContext");
    }

    mSurfaceBuffers = new VideoSurfaceBuffer [mNumSurfaces];
    if (mSurfaceBuffers == NULL) {
        return DECODE_MEMORY_FAIL;
    }
    initSurfaceBuffer(true);

    if ((int32_t)profile == VAProfileSoftwareDecoding) {
        // derive user pointer from surface for direct access
        status = mapSurface();
        CHECK_STATUS("mapSurface")
    }

    setRotationDegrees(mConfigBuffer.rotationDegrees);

    mVAStarted = true;
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::terminateVA(void) {
    mSignalBufferSize = 0;
    for (int i = 0; i < MAX_GRAPHIC_BUFFER_NUM; i++) {
         mSignalBufferPre[i] = NULL;
    }

    if (mVAStarted == false) {
        // VA hasn't been started yet
        return DECODE_SUCCESS;
    }

    if (mSurfaceBuffers) {
        for (int32_t i = 0; i < mNumSurfaces; i++) {
            if (mSurfaceBuffers[i].renderBuffer.rawData) {
                if (mSurfaceBuffers[i].renderBuffer.rawData->data) {
                    delete [] mSurfaceBuffers[i].renderBuffer.rawData->data;
                }
                delete mSurfaceBuffers[i].renderBuffer.rawData;
            }
            if (mSurfaceBuffers[i].mappedData) {
                // don't  delete data pointer as it is mapped from surface
                delete mSurfaceBuffers[i].mappedData;
            }
        }
        delete [] mSurfaceBuffers;
        mSurfaceBuffers = NULL;
    }

    if (mVASurfaceAttrib) {
        if (mVASurfaceAttrib->buffers) free(mVASurfaceAttrib->buffers);
        delete mVASurfaceAttrib;
        mVASurfaceAttrib = NULL;
    }


    if (mSurfaceUserPtr) {
        delete [] mSurfaceUserPtr;
        mSurfaceUserPtr = NULL;
    }

    if (mSurfaces)
    {
        vaDestroySurfaces(mVADisplay, mSurfaces, mNumSurfaces + mNumExtraSurfaces);
        delete [] mSurfaces;
        mSurfaces = NULL;
    }

    if (mVAContext != VA_INVALID_ID) {
         vaDestroyContext(mVADisplay, mVAContext);
         mVAContext = VA_INVALID_ID;
    }

    if (mVAConfig != VA_INVALID_ID) {
        vaDestroyConfig(mVADisplay, mVAConfig);
        mVAConfig = VA_INVALID_ID;
    }

    if (mVADisplay) {
        vaTerminate(mVADisplay);
        mVADisplay = NULL;
    }

    if (mDisplay) {
#ifndef USE_HYBRID_DRIVER
        delete mDisplay;
#endif
        mDisplay = NULL;
    }

    mVAStarted = false;
    mInitialized = false;
    mErrReportEnabled = false;
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::parseBuffer(uint8_t *buffer, int32_t size, bool config, void** vbpData) {
     // DON'T check if mVAStarted == true
    if (mParserHandle == NULL) {
        return DECODE_NO_PARSER;
    }

    uint32_t vbpStatus;
    if (buffer == NULL || size <= 0) {
        return DECODE_INVALID_DATA;
    }

    uint8_t configFlag = config ? 1 : 0;
    vbpStatus = mParserParse(mParserHandle, buffer, size, configFlag);
    CHECK_VBP_STATUS("vbp_parse");

    vbpStatus = mParserQuery(mParserHandle, vbpData);
    CHECK_VBP_STATUS("vbp_query");

    return DECODE_SUCCESS;
}



Decode_Status VideoDecoderBase::mapSurface(void) {
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAImage image;
    uint8_t *userPtr;
    mSurfaceUserPtr = new uint8_t* [mNumSurfaces];
    if (mSurfaceUserPtr == NULL) {
        return DECODE_MEMORY_FAIL;
    }

    for (int32_t i = 0; i< mNumSurfaces; i++) {
        vaStatus = vaDeriveImage(mVADisplay, mSurfaces[i], &image);
        CHECK_VA_STATUS("vaDeriveImage");
        vaStatus = vaMapBuffer(mVADisplay, image.buf, (void**)&userPtr);
        CHECK_VA_STATUS("vaMapBuffer");
        mSurfaceUserPtr[i] = userPtr;
        mSurfaceBuffers[i].mappedData = new VideoFrameRawData;
        if (mSurfaceBuffers[i].mappedData == NULL) {
            return DECODE_MEMORY_FAIL;
        }
        mSurfaceBuffers[i].mappedData->own = false; // derived from surface so can't be released
        mSurfaceBuffers[i].mappedData->data = NULL;  // specified during acquireSurfaceBuffer
        mSurfaceBuffers[i].mappedData->fourcc = image.format.fourcc;
        mSurfaceBuffers[i].mappedData->width = mVideoFormatInfo.width;
        mSurfaceBuffers[i].mappedData->height = mVideoFormatInfo.height;
        mSurfaceBuffers[i].mappedData->size = image.data_size;
        for (int pi = 0; pi < 3; pi++) {
            mSurfaceBuffers[i].mappedData->pitch[pi] = image.pitches[pi];
            mSurfaceBuffers[i].mappedData->offset[pi] = image.offsets[pi];
        }
        // debug information
        if (image.pitches[0] != image.pitches[1] ||
            image.width != mVideoFormatInfo.width ||
            image.height != mVideoFormatInfo.height ||
            image.offsets[0] != 0) {
            WTRACE("Unexpected VAImage format, w = %d, h = %d, offset = %d", image.width, image.height, image.offsets[0]);
        }
        // TODO: do we need to unmap buffer?
        //vaStatus = vaUnmapBuffer(mVADisplay, image.buf);
        //CHECK_VA_STATUS("vaMapBuffer");
        vaStatus = vaDestroyImage(mVADisplay,image.image_id);
        CHECK_VA_STATUS("vaDestroyImage");

    }
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::getRawDataFromSurface(VideoRenderBuffer *renderBuffer, uint8_t *pRawData, uint32_t *pSize, bool internal) {
    if (internal) {
        if (mAcquiredBuffer == NULL) {
            return DECODE_FAIL;
        }
        renderBuffer = &(mAcquiredBuffer->renderBuffer);
    }

    VAStatus vaStatus;
    VAImageFormat imageFormat;
    VAImage vaImage;
    vaStatus = vaSyncSurface(renderBuffer->display, renderBuffer->surface);
    CHECK_VA_STATUS("vaSyncSurface");

    vaStatus = vaDeriveImage(renderBuffer->display, renderBuffer->surface, &vaImage);
    CHECK_VA_STATUS("vaDeriveImage");

    void *pBuf = NULL;
    vaStatus = vaMapBuffer(renderBuffer->display, vaImage.buf, &pBuf);
    CHECK_VA_STATUS("vaMapBuffer");


    // size in NV12 format
    uint32_t cropWidth = mVideoFormatInfo.width - (mVideoFormatInfo.cropLeft + mVideoFormatInfo.cropRight);
    uint32_t cropHeight = mVideoFormatInfo.height - (mVideoFormatInfo.cropBottom + mVideoFormatInfo.cropTop);
    int32_t size = cropWidth  * cropHeight * 3 / 2;

    if (internal) {
        VideoFrameRawData *rawData = NULL;
        if (renderBuffer->rawData == NULL) {
            rawData = new VideoFrameRawData;
            if (rawData == NULL) {
                return DECODE_MEMORY_FAIL;
            }
            memset(rawData, 0, sizeof(VideoFrameRawData));
            renderBuffer->rawData = rawData;
        } else {
            rawData = renderBuffer->rawData;
        }

        if (rawData->data != NULL && rawData->size != size) {
            delete [] rawData->data;
            rawData->data = NULL;
            rawData->size = 0;
        }
        if (rawData->data == NULL) {
            rawData->data = new uint8_t [size];
            if (rawData->data == NULL) {
                return DECODE_MEMORY_FAIL;
            }
        }

        rawData->own = true; // allocated by this library
        rawData->width = cropWidth;
        rawData->height = cropHeight;
        rawData->pitch[0] = cropWidth;
        rawData->pitch[1] = cropWidth;
        rawData->pitch[2] = 0;  // interleaved U/V, two planes
        rawData->offset[0] = 0;
        rawData->offset[1] = cropWidth * cropHeight;
        rawData->offset[2] = cropWidth * cropHeight * 3 / 2;
        rawData->size = size;
        rawData->fourcc = 'NV12';

        pRawData = rawData->data;
    } else {
        *pSize = size;
    }

    if (size == (int32_t)vaImage.data_size) {
#ifdef  __SSE4_1__
        stream_memcpy(pRawData, pBuf, size);
#else
        memcpy(pRawData, pBuf, size);
#endif
    } else {
        // copy Y data
        uint8_t *src = (uint8_t*)pBuf;
        uint8_t *dst = pRawData;
        uint32_t row = 0;
        for (row = 0; row < cropHeight; row++) {
#ifdef  __SSE4_1__
            stream_memcpy(dst, src, cropWidth);
#else
            memcpy(dst, src, cropWidth);
#endif
            dst += cropWidth;
            src += vaImage.pitches[0];
        }
        // copy interleaved V and  U data
        src = (uint8_t*)pBuf + vaImage.offsets[1];
        for (row = 0; row < cropHeight / 2; row++) {
#ifdef  __SSE4_1__
            stream_memcpy(dst, src, cropWidth);
#else
            memcpy(dst, src, cropWidth);
#endif
            dst += cropWidth;
            src += vaImage.pitches[1];
        }
    }

    vaStatus = vaUnmapBuffer(renderBuffer->display, vaImage.buf);
    CHECK_VA_STATUS("vaUnmapBuffer");

    vaStatus = vaDestroyImage(renderBuffer->display, vaImage.image_id);
    CHECK_VA_STATUS("vaDestroyImage");

    return DECODE_SUCCESS;
}

void VideoDecoderBase::initSurfaceBuffer(bool reset) {
    bool useGraphicBuffer = mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER;
    if (useGraphicBuffer && reset) {
        pthread_mutex_lock(&mLock);
    }
    for (int32_t i = 0; i < mNumSurfaces; i++) {
        mSurfaceBuffers[i].renderBuffer.display = mVADisplay;
        mSurfaceBuffers[i].renderBuffer.surface = VA_INVALID_SURFACE;  // set in acquireSurfaceBuffer
        mSurfaceBuffers[i].renderBuffer.flag = 0;
        mSurfaceBuffers[i].renderBuffer.scanFormat = VA_FRAME_PICTURE;
        mSurfaceBuffers[i].renderBuffer.timeStamp = 0;
        mSurfaceBuffers[i].referenceFrame = false;
        mSurfaceBuffers[i].asReferernce= false;
        mSurfaceBuffers[i].pictureOrder = 0;
        mSurfaceBuffers[i].next = NULL;
        if (reset == true) {
            mSurfaceBuffers[i].renderBuffer.rawData = NULL;
            mSurfaceBuffers[i].mappedData = NULL;
        }
        if (useGraphicBuffer) {
            if (reset) {
               mSurfaceBuffers[i].renderBuffer.graphicBufferHandle = mConfigBuffer.graphicBufferHandler[i];
               mSurfaceBuffers[i].renderBuffer.renderDone = false; //default false
               for (uint32_t j = 0; j < mSignalBufferSize; j++) {
                   if(mSignalBufferPre[j] != NULL && mSignalBufferPre[j] == mSurfaceBuffers[i].renderBuffer.graphicBufferHandle) {
                      mSurfaceBuffers[i].renderBuffer.renderDone = true;
                      VTRACE("initSurfaceBuffer set renderDone = true index = %d", i);
                      mSignalBufferPre[j] = NULL;
                      break;
                   }
               }
            } else {
               mSurfaceBuffers[i].renderBuffer.renderDone = false;
            }
        } else {
            mSurfaceBuffers[i].renderBuffer.graphicBufferHandle = NULL;
            mSurfaceBuffers[i].renderBuffer.renderDone = true;
        }
        mSurfaceBuffers[i].renderBuffer.graphicBufferIndex = i;
    }

    if (useGraphicBuffer && reset) {
        mInitialized = true;
        mSignalBufferSize = 0;
        pthread_mutex_unlock(&mLock);
    }
}

Decode_Status VideoDecoderBase::signalRenderDone(void * graphichandler) {
    if (graphichandler == NULL) {
        return DECODE_SUCCESS;
    }
    pthread_mutex_lock(&mLock);
    int i = 0;
    if (!mInitialized) {
        if (mSignalBufferSize >= MAX_GRAPHIC_BUFFER_NUM) {
            pthread_mutex_unlock(&mLock);
            return DECODE_INVALID_DATA;
        }
        mSignalBufferPre[mSignalBufferSize++] = graphichandler;
        VTRACE("SignalRenderDoneFlag mInitialized = false graphichandler = %p, mSignalBufferSize = %d", graphichandler, mSignalBufferSize);
    } else {
        if (!(mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER)) {
            pthread_mutex_unlock(&mLock);
            return DECODE_SUCCESS;
        }
        for (i = 0; i < mNumSurfaces; i++) {
            if (mSurfaceBuffers[i].renderBuffer.graphicBufferHandle == graphichandler) {
                mSurfaceBuffers[i].renderBuffer.renderDone = true;
                VTRACE("SignalRenderDoneFlag mInitialized = true index = %d", i);
               break;
           }
        }
    }
    pthread_mutex_unlock(&mLock);

    return DECODE_SUCCESS;

}

void VideoDecoderBase::querySurfaceRenderStatus(VideoSurfaceBuffer* surface) {
    VASurfaceStatus surfStat = VASurfaceReady;
    VAStatus    vaStat = VA_STATUS_SUCCESS;

    if (!surface) {
        LOGW("SurfaceBuffer not ready yet");
        return;
    }
    surface->renderBuffer.driverRenderDone = true;

#ifndef USE_GEN_HW
    if (surface->renderBuffer.surface != VA_INVALID_SURFACE &&
       (mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER)) {

        vaStat = vaQuerySurfaceStatus(mVADisplay, surface->renderBuffer.surface, &surfStat);

        if ((vaStat == VA_STATUS_SUCCESS) && (surfStat != VASurfaceReady))
            surface->renderBuffer.driverRenderDone = false;

    }
#endif

}

// This function should be called before start() to load different type of parsers
#if (defined USE_AVC_SHORT_FORMAT || defined USE_SLICE_HEADER_PARSING)
Decode_Status VideoDecoderBase::setParserType(_vbp_parser_type type) {
    if ((int32_t)type != VBP_INVALID) {
        ITRACE("Parser Type = %d", (int32_t)type);
        mParserType = type;
        return DECODE_SUCCESS;
    } else {
        ETRACE("Invalid parser type = %d", (int32_t)type);
        return DECODE_NO_PARSER;
    }
}

Decode_Status VideoDecoderBase::updateBuffer(uint8_t *buffer, int32_t size, void** vbpData) {
    if (mParserHandle == NULL) {
        return DECODE_NO_PARSER;
    }

    uint32_t vbpStatus;
    if (buffer == NULL || size <= 0) {
        return DECODE_INVALID_DATA;
    }

    vbpStatus = mParserUpdate(mParserHandle, buffer, size, vbpData);
    CHECK_VBP_STATUS("vbp_update");

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::queryBuffer(void** vbpData) {
    if (mParserHandle == NULL) {
        return DECODE_NO_PARSER;
    }

    uint32_t vbpStatus;
    vbpStatus = mParserQuery(mParserHandle, vbpData);
    CHECK_VBP_STATUS("vbp_query");

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::getCodecSpecificConfigs(VAProfile profile, VAConfigID *config) {
    VAStatus vaStatus;
    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    attrib.value = VA_RT_FORMAT_YUV420;

    if (config == NULL) {
        ETRACE("Invalid parameter!");
        return DECODE_FAIL;
    }

    vaStatus = vaCreateConfig(
            mVADisplay,
            profile,
            VAEntrypointVLD,
            &attrib,
            1,
            config);

    CHECK_VA_STATUS("vaCreateConfig");

    return DECODE_SUCCESS;
}
#endif
Decode_Status VideoDecoderBase::checkHardwareCapability() {
    return DECODE_SUCCESS;
}

void VideoDecoderBase::drainDecodingErrors(VideoErrorBuffer *outErrBuf, VideoRenderBuffer *currentSurface) {
    if (mErrReportEnabled && outErrBuf && currentSurface) {
        memcpy(outErrBuf, &(currentSurface->errBuf), sizeof(VideoErrorBuffer));

        currentSurface->errBuf.errorNumber = 0;
        currentSurface->errBuf.timeStamp = INVALID_PTS;
    }
    if (outErrBuf)
        VTRACE("%s: error number is %d", __FUNCTION__, outErrBuf->errorNumber);
}

void VideoDecoderBase::fillDecodingErrors(VideoRenderBuffer *currentSurface) {
    VAStatus ret;

    if (mErrReportEnabled) {
        currentSurface->errBuf.timeStamp = currentSurface->timeStamp;
        // TODO: is 10 a suitable number?
        VASurfaceDecodeMBErrors *err_drv_output = NULL;
        ret = vaQuerySurfaceError(mVADisplay, currentSurface->surface, VA_STATUS_ERROR_DECODING_ERROR, (void **)&err_drv_output);
        if (ret || !err_drv_output) {
            WTRACE("vaQuerySurfaceError failed.");
            return;
        }

        int offset =  0x1 & currentSurface->errBuf.errorNumber;// offset is either 0 or 1
        for (int i = 0; i < MAX_ERR_NUM - offset; i++) {
            if (err_drv_output[i].status != -1) {
                currentSurface->errBuf.errorNumber++;
                currentSurface->errBuf.errorArray[i + offset].type = DecodeMBError;
                currentSurface->errBuf.errorArray[i + offset].error_data.mb_pos.start_mb = err_drv_output[i].start_mb;
                currentSurface->errBuf.errorArray[i + offset].error_data.mb_pos.end_mb = err_drv_output[i].end_mb;
                currentSurface->errBuf.errorArray[i + offset].num_mbs = err_drv_output[i].end_mb - err_drv_output[i].start_mb + 1;
                ITRACE("Error Index[%d]: type = %d, start_mb = %d, end_mb = %d",
                    currentSurface->errBuf.errorNumber - 1,
                    currentSurface->errBuf.errorArray[i + offset].type,
                    currentSurface->errBuf.errorArray[i + offset].error_data.mb_pos.start_mb,
                    currentSurface->errBuf.errorArray[i + offset].error_data.mb_pos.end_mb);
            } else break;
        }
        ITRACE("%s: error number of current surface is %d, timestamp @%llu",
            __FUNCTION__, currentSurface->errBuf.errorNumber, currentSurface->timeStamp);
    }
}

void VideoDecoderBase::setRotationDegrees(int32_t rotationDegrees) {
    if (mRotationDegrees == rotationDegrees) {
        return;
    }

    ITRACE("set new rotation degree: %d", rotationDegrees);
    VADisplayAttribute rotate;
    rotate.type = VADisplayAttribRotation;
    rotate.value = VA_ROTATION_NONE;
    if (rotationDegrees == 0)
        rotate.value = VA_ROTATION_NONE;
    else if (rotationDegrees == 90)
        rotate.value = VA_ROTATION_90;
    else if (rotationDegrees == 180)
        rotate.value = VA_ROTATION_180;
    else if (rotationDegrees == 270)
        rotate.value = VA_ROTATION_270;

    VAStatus ret = vaSetDisplayAttributes(mVADisplay, &rotate, 1);
    if (ret) {
        ETRACE("Failed to set rotation degree.");
    }
    mRotationDegrees = rotationDegrees;
}

void VideoDecoderBase::setRenderRect() {

    if (!mVADisplay)
        return;

    VAStatus ret;
    VARectangle rect;
    rect.x = mVideoFormatInfo.cropLeft;
    rect.y = mVideoFormatInfo.cropTop;
    rect.width = mVideoFormatInfo.width - (mVideoFormatInfo.cropLeft + mVideoFormatInfo.cropRight);
    rect.height = mVideoFormatInfo.height - (mVideoFormatInfo.cropBottom + mVideoFormatInfo.cropTop);

    VADisplayAttribute render_rect;
    render_rect.type = VADisplayAttribRenderRect;
    render_rect.value = (long)&rect;

    ret = vaSetDisplayAttributes(mVADisplay, &render_rect, 1);
    if (ret) {
        ETRACE("Failed to set rotation degree.");
    }
}
