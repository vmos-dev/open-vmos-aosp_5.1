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

#include "VideoDecoderAVC.h"
#include "VideoDecoderTrace.h"
#include <string.h>
#include <cutils/properties.h>

// Macros for actual buffer needed calculation
#define WIDI_CONSUMED   6
#define HDMI_CONSUMED   2
#define NW_CONSUMED     2
#define POC_DEFAULT     0x7FFFFFFF

VideoDecoderAVC::VideoDecoderAVC(const char *mimeType)
    : VideoDecoderBase(mimeType, VBP_H264),
      mToggleDPB(0),
      mErrorConcealment(false){

    invalidateDPB(0);
    invalidateDPB(1);
    mLastPictureFlags = VA_PICTURE_H264_INVALID;
}

VideoDecoderAVC::~VideoDecoderAVC() {
    stop();
}

Decode_Status VideoDecoderAVC::start(VideoConfigBuffer *buffer) {
    Decode_Status status;

    status = VideoDecoderBase::start(buffer);
    CHECK_STATUS("VideoDecoderBase::start");

    // We don't want base class to manage reference.
    VideoDecoderBase::ManageReference(false);
    // output by picture order count
    VideoDecoderBase::setOutputMethod(OUTPUT_BY_POC);

    mErrorConcealment = buffer->flag & WANT_ERROR_CONCEALMENT;
    if (buffer->data == NULL || buffer->size == 0) {
        WTRACE("No config data to start VA.");
        if ((buffer->flag & HAS_SURFACE_NUMBER) && (buffer->flag & HAS_VA_PROFILE)) {
            ITRACE("Used client supplied profile and surface to start VA.");
            return VideoDecoderBase::setupVA(buffer->surfaceNumber, buffer->profile);
        }
        return DECODE_SUCCESS;
    }

    vbp_data_h264 *data = NULL;
    status = VideoDecoderBase::parseBuffer(buffer->data, buffer->size, true, (void**)&data);
    CHECK_STATUS("VideoDecoderBase::parseBuffer");

    status = startVA(data);
    return status;
}

void VideoDecoderAVC::stop(void) {
    // drop the last  frame and ignore return value
    endDecodingFrame(true);
    VideoDecoderBase::stop();
    invalidateDPB(0);
    invalidateDPB(1);
    mToggleDPB = 0;
    mErrorConcealment = false;
    mLastPictureFlags = VA_PICTURE_H264_INVALID;
}

void VideoDecoderAVC::flush(void) {
    // drop the frame and ignore return value
    VideoDecoderBase::flush();
    invalidateDPB(0);
    invalidateDPB(1);
    mToggleDPB = 0;
    mLastPictureFlags = VA_PICTURE_H264_INVALID;
}

Decode_Status VideoDecoderAVC::decode(VideoDecodeBuffer *buffer) {
    Decode_Status status;
    vbp_data_h264 *data = NULL;
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }
    status =  VideoDecoderBase::parseBuffer(
            buffer->data,
            buffer->size,
            false,
            (void**)&data);
    CHECK_STATUS("VideoDecoderBase::parseBuffer");

    if (!mVAStarted) {
         if (data->has_sps && data->has_pps) {
            status = startVA(data);
            CHECK_STATUS("startVA");
        } else {
            WTRACE("Can't start VA as either SPS or PPS is still not available.");
            return DECODE_SUCCESS;
        }
    }

    VideoDecoderBase::setRotationDegrees(buffer->rotationDegrees);

    status = decodeFrame(buffer, data);
    if (status == DECODE_MULTIPLE_FRAME) {
        buffer->ext = &mExtensionBuffer;
        mExtensionBuffer.extType = PACKED_FRAME_TYPE;
        mExtensionBuffer.extSize = sizeof(mPackedFrame);
        mExtensionBuffer.extData = (uint8_t*)&mPackedFrame;
    }
    return status;
}

Decode_Status VideoDecoderAVC::decodeFrame(VideoDecodeBuffer *buffer, vbp_data_h264 *data) {
    Decode_Status status;
    if (data->has_sps == 0 || data->has_pps == 0) {
        return DECODE_NO_CONFIG;
    }

    mVideoFormatInfo.flags = 0;
    uint32_t fieldFlags = 0;
    for (unsigned int i = 0; i < data->num_pictures; i++) {
        VAPictureH264 &pic = data->pic_data[i].pic_parms->CurrPic;
        fieldFlags |= pic.flags;
        // Don't remove the following codes, it can be enabled for debugging DPB.
#if 0
        VTRACE("%d: decoding frame %.2f, poc top = %d, poc bottom = %d, flags = %d,  reference = %d",
                i,
                buffer->timeStamp/1E6,
                pic.TopFieldOrderCnt,
                pic.BottomFieldOrderCnt,
                pic.flags,
                (pic.flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
                (pic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE));
#endif
    }
    int32_t topField = fieldFlags & VA_PICTURE_H264_TOP_FIELD;
    int32_t botField = fieldFlags & VA_PICTURE_H264_BOTTOM_FIELD;
    if ((topField == 0 && botField != 0) || (topField != 0 && botField == 0)) {
        mVideoFormatInfo.flags |= IS_SINGLE_FIELD;
    }

    if (data->new_sps || data->new_pps) {
        status = handleNewSequence(data);
        CHECK_STATUS("handleNewSequence");
    }

    if (isWiDiStatusChanged()) {
        mSizeChanged = false;
        flushSurfaceBuffers();
        return DECODE_FORMAT_CHANGE;
    }

    // first pic_data always exists, check if any slice is parsed
    if (data->pic_data[0].num_slices == 0) {
        ITRACE("No slice available for decoding.");
        status = mSizeChanged ? DECODE_FORMAT_CHANGE : DECODE_SUCCESS;
        mSizeChanged = false;
        return status;
    }

    uint64_t lastPTS = mCurrentPTS;
    mCurrentPTS = buffer->timeStamp;
    //if (lastPTS != mCurrentPTS) {
    if (isNewFrame(data, lastPTS == mCurrentPTS)) {
        if (mLowDelay) {
            // start decoding a new frame
            status = beginDecodingFrame(data);
            if (status != DECODE_SUCCESS) {
                Decode_Status st = status;
                // finish decoding the last frame if
                // encounter error when decode the new frame
                status = endDecodingFrame(false);
                CHECK_STATUS("endDecodingFrame");
                return st;
            }
        }

        // finish decoding the last frame
        status = endDecodingFrame(false);
        CHECK_STATUS("endDecodingFrame");

        if (!mLowDelay) {
            // start decoding a new frame
            status = beginDecodingFrame(data);
            CHECK_STATUS("beginDecodingFrame");
        }
    } else {
        status = continueDecodingFrame(data);
        CHECK_STATUS("continueDecodingFrame");
    }

    // HAS_COMPLETE_FRAME is not reliable as it may indicate end of a field
#if 0
    if (buffer->flag & HAS_COMPLETE_FRAME) {
        // finish decoding current frame
        status = endDecodingFrame(false);
        CHECK_STATUS("endDecodingFrame");
    }
#endif
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVC::beginDecodingFrame(vbp_data_h264 *data) {
    Decode_Status status;

    status = acquireSurfaceBuffer();
    CHECK_STATUS("acquireSurfaceBuffer");
    VAPictureH264 *picture = &(data->pic_data[0].pic_parms->CurrPic);
    if ((picture->flags  & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
        (picture->flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)) {
        mAcquiredBuffer->referenceFrame = true;
    } else {
        mAcquiredBuffer->referenceFrame = false;
    }
    // set asReference in updateDPB

    if (picture->flags & VA_PICTURE_H264_TOP_FIELD) {
        mAcquiredBuffer->renderBuffer.scanFormat = VA_BOTTOM_FIELD | VA_TOP_FIELD;
    } else {
        mAcquiredBuffer->renderBuffer.scanFormat = VA_FRAME_PICTURE;
    }

    // TODO: Set the discontinuity flag
    mAcquiredBuffer->renderBuffer.flag = 0;
    mAcquiredBuffer->renderBuffer.timeStamp = mCurrentPTS;
    mAcquiredBuffer->pictureOrder = getPOC(picture);

    if (mSizeChanged) {
        mAcquiredBuffer->renderBuffer.flag |= IS_RESOLUTION_CHANGE;
        mSizeChanged = false;
    }

    status  = continueDecodingFrame(data);
    // surface buffer is released if decode fails
    return status;
}


Decode_Status VideoDecoderAVC::continueDecodingFrame(vbp_data_h264 *data) {
    Decode_Status status;
    vbp_picture_data_h264 *picData = data->pic_data;

    // TODO: remove these debugging codes
    if (mAcquiredBuffer == NULL || mAcquiredBuffer->renderBuffer.surface == VA_INVALID_SURFACE) {
        ETRACE("mAcquiredBuffer is NULL. Implementation bug.");
        return DECODE_FAIL;
    }
    for (uint32_t picIndex = 0; picIndex < data->num_pictures; picIndex++, picData++) {
        // sanity check
        if (picData == NULL || picData->pic_parms == NULL || picData->slc_data == NULL || picData->num_slices == 0) {
            return DECODE_PARSER_FAIL;
        }

        if (picIndex > 0 &&
            (picData->pic_parms->CurrPic.flags & (VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD)) == 0) {
            // it is a packed frame buffer
            vbp_picture_data_h264 *lastPic = &data->pic_data[picIndex - 1];
            vbp_slice_data_h264 *sliceData = &(lastPic->slc_data[lastPic->num_slices - 1]);
            mPackedFrame.offSet = sliceData->slice_size + sliceData->slice_offset;
            mPackedFrame.timestamp = mCurrentPTS; // use the current time stamp for the packed frame
            ITRACE("slice data offset= %d, size = %d", sliceData->slice_offset, sliceData->slice_size);
            return DECODE_MULTIPLE_FRAME;
        }

        for (uint32_t sliceIndex = 0; sliceIndex < picData->num_slices; sliceIndex++) {
            status = decodeSlice(data, picIndex, sliceIndex);
            if (status != DECODE_SUCCESS) {
                endDecodingFrame(true);
                // TODO: this is new code
                // remove current frame from DPB as it can't be decoded.
                removeReferenceFromDPB(picData->pic_parms);
                return status;
            }
        }
    }
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVC::decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex) {
    Decode_Status status;
    VAStatus vaStatus;
    uint32_t bufferIDCount = 0;
    // maximum 4 buffers to render a slice: picture parameter, IQMatrix, slice parameter, slice data
    VABufferID bufferIDs[4];

    vbp_picture_data_h264 *picData = &(data->pic_data[picIndex]);
    vbp_slice_data_h264 *sliceData = &(picData->slc_data[sliceIndex]);
    VAPictureParameterBufferH264 *picParam = picData->pic_parms;
    VASliceParameterBufferH264 *sliceParam = &(sliceData->slc_parms);

    if (sliceParam->first_mb_in_slice == 0 || mDecodingFrame == false) {
        // either condition indicates start of a new frame
        if (sliceParam->first_mb_in_slice != 0) {
            WTRACE("The first slice is lost.");
            // TODO: handle the first slice lost
        }
        if (mDecodingFrame) {
            // interlace content, complete decoding the first field
            vaStatus = vaEndPicture(mVADisplay, mVAContext);
            CHECK_VA_STATUS("vaEndPicture");

            // for interlace content, top field may be valid only after the second field is parsed
            int32_t poc = getPOC(&(picParam->CurrPic));
            if (poc < mAcquiredBuffer->pictureOrder) {
                mAcquiredBuffer->pictureOrder = poc;
            }
        }

        // Check there is no reference frame loss before decoding a frame

        // Update  the reference frames and surface IDs for DPB and current frame
        status = updateDPB(picParam);
        CHECK_STATUS("updateDPB");

#ifndef USE_AVC_SHORT_FORMAT
        //We have to provide a hacked DPB rather than complete DPB for libva as workaround
        status = updateReferenceFrames(picData);
        CHECK_STATUS("updateReferenceFrames");
#endif
        vaStatus = vaBeginPicture(mVADisplay, mVAContext, mAcquiredBuffer->renderBuffer.surface);
        CHECK_VA_STATUS("vaBeginPicture");

        // start decoding a frame
        mDecodingFrame = true;

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAPictureParameterBufferType,
            sizeof(VAPictureParameterBufferH264),
            1,
            picParam,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreatePictureParameterBuffer");
        bufferIDCount++;

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAIQMatrixBufferType,
            sizeof(VAIQMatrixBufferH264),
            1,
            data->IQ_matrix_buf,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateIQMatrixBuffer");
        bufferIDCount++;
    }

#ifndef USE_AVC_SHORT_FORMAT

    status = setReference(sliceParam);
    CHECK_STATUS("setReference");

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceParameterBufferType,
        sizeof(VASliceParameterBufferH264),
        1,
        sliceParam,
        &bufferIDs[bufferIDCount]);
#else
    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceParameterBufferType,
        sizeof(VASliceParameterBufferH264Base),
        1,
        sliceParam,
        &bufferIDs[bufferIDCount]);
#endif
    CHECK_VA_STATUS("vaCreateSliceParameterBuffer");
    bufferIDCount++;

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceDataBufferType,
        sliceData->slice_size, //size
        1,        //num_elements
        sliceData->buffer_addr + sliceData->slice_offset,
        &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreateSliceDataBuffer");
    bufferIDCount++;

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        bufferIDs,
        bufferIDCount);
    CHECK_VA_STATUS("vaRenderPicture");

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVC::setReference(VASliceParameterBufferH264 *sliceParam) {
    int32_t numList = 1;
    // TODO: set numList to 0 if it is I slice
    if (sliceParam->slice_type == 1 || sliceParam->slice_type == 6) {
        // B slice
        numList = 2;
    }

    int32_t activeMinus1 = sliceParam->num_ref_idx_l0_active_minus1;
    VAPictureH264 *ref = sliceParam->RefPicList0;

    for (int32_t i = 0; i < numList; i++) {
        if (activeMinus1 >= REF_LIST_SIZE) {
            ETRACE("Invalid activeMinus1 (%d)", activeMinus1);
            return DECODE_PARSER_FAIL;
        }
        for (int32_t j = 0; j <= activeMinus1; j++, ref++) {
            if (!(ref->flags & VA_PICTURE_H264_INVALID)) {
                ref->picture_id = findSurface(ref);
                if (ref->picture_id == VA_INVALID_SURFACE) {
                    // Error DecodeRefMissing is counted once even there're multiple
                    mAcquiredBuffer->renderBuffer.errBuf.errorNumber = 1;
                    mAcquiredBuffer->renderBuffer.errBuf.errorArray[0].type = DecodeRefMissing;

                    if (mLastReference) {
                        WTRACE("Reference frame %d is missing. Use last reference", getPOC(ref));
                        ref->picture_id = mLastReference->renderBuffer.surface;
                    } else {
                        ETRACE("Reference frame %d is missing. Stop decoding.", getPOC(ref));
                        return DECODE_NO_REFERENCE;
                    }
                }
            }
        }
        activeMinus1 = sliceParam->num_ref_idx_l1_active_minus1;
        ref = sliceParam->RefPicList1;
    }
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVC::updateDPB(VAPictureParameterBufferH264 *picParam) {
    clearAsReference(mToggleDPB);
    // pointer to toggled DPB (new)
    DecodedPictureBuffer *dpb = mDPBs[!mToggleDPB];
    VAPictureH264 *ref = picParam->ReferenceFrames;

    // update current picture ID
    picParam->CurrPic.picture_id = mAcquiredBuffer->renderBuffer.surface;

    // build new DPB
    for (int32_t i = 0; i < MAX_REF_NUMBER; i++, ref++) {
        if (ref->flags & VA_PICTURE_H264_INVALID) {
            continue;
        }
#ifdef USE_AVC_SHORT_FORMAT
        ref->picture_id = findSurface(ref);
#endif
        dpb->poc = getPOC(ref);
        // looking for the latest ref frame in the DPB with specified POC, in case frames have same POC
        dpb->surfaceBuffer = findRefSurfaceBuffer(ref);
        if (dpb->surfaceBuffer == NULL) {
            ETRACE("Reference frame %d is missing for current frame %d", dpb->poc, getPOC(&(picParam->CurrPic)));
            // Error DecodeRefMissing is counted once even there're multiple
            mAcquiredBuffer->renderBuffer.errBuf.errorNumber = 1;
            mAcquiredBuffer->renderBuffer.errBuf.errorArray[0].type = DecodeRefMissing;
            if (dpb->poc == getPOC(&(picParam->CurrPic))) {
                WTRACE("updateDPB: Using the current picture for missing reference.");
                dpb->surfaceBuffer = mAcquiredBuffer;
            } else if (mLastReference) {
                WTRACE("updateDPB: Use last reference frame %d for missing reference.", mLastReference->pictureOrder);
                // TODO: this is new code for error resilience
                dpb->surfaceBuffer = mLastReference;
            } else {
                WTRACE("updateDPB: Unable to recover the missing reference frame.");
                // continue buillding DPB without updating dpb pointer.
                continue;
                // continue building DPB as this reference may not be actually used.
                // especially happen after seeking to a non-IDR I frame.
                //return DECODE_NO_REFERENCE;
            }
        }
        if (dpb->surfaceBuffer) {
            // this surface is used as reference
            dpb->surfaceBuffer->asReferernce = true;
        }
        dpb++;
    }

    // add current frame to DPB if it  is a reference frame
    if ((picParam->CurrPic.flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
        (picParam->CurrPic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)) {
        dpb->poc = getPOC(&(picParam->CurrPic));
        dpb->surfaceBuffer = mAcquiredBuffer;
        dpb->surfaceBuffer->asReferernce = true;
    }
    // invalidate the current used DPB
    invalidateDPB(mToggleDPB);
    mToggleDPB = !mToggleDPB;
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVC::updateReferenceFrames(vbp_picture_data_h264 *picData) {
    bool found = false;
    uint32_t flags = 0;
    VAPictureParameterBufferH264 *picParam = picData->pic_parms;
    VASliceParameterBufferH264 *sliceParam = NULL;
    uint8_t activeMinus1 = 0;
    VAPictureH264 *refList = NULL;
    VAPictureH264 *dpb = picParam->ReferenceFrames;
    VAPictureH264 *refFrame = NULL;

    for(int i = 0; i < picParam->num_ref_frames; i++) {
        dpb->picture_id = findSurface(dpb);
        dpb++;
    }

    return DECODE_SUCCESS;

    // invalidate DPB in the picture buffer
    memset(picParam->ReferenceFrames, 0xFF, sizeof(picParam->ReferenceFrames));
    picParam->num_ref_frames = 0;

    // update DPB  from the reference list in each slice.
    for (uint32_t slice = 0; slice < picData->num_slices; slice++) {
        sliceParam = &(picData->slc_data[slice].slc_parms);

        for (int32_t list = 0; list < 2; list++) {
            refList = (list == 0) ? sliceParam->RefPicList0 :
                                    sliceParam->RefPicList1;
            activeMinus1 = (list == 0) ? sliceParam->num_ref_idx_l0_active_minus1 :
                                         sliceParam->num_ref_idx_l1_active_minus1;
            if (activeMinus1 >= REF_LIST_SIZE) {
                return DECODE_PARSER_FAIL;
            }
            for (uint8_t item = 0; item < (uint8_t)(activeMinus1 + 1); item++, refList++) {
                if (refList->flags & VA_PICTURE_H264_INVALID) {
                    break;
                }
                found = false;
                refFrame = picParam->ReferenceFrames;
                for (uint8_t frame = 0; frame < picParam->num_ref_frames; frame++, refFrame++) {
                    if (refFrame->TopFieldOrderCnt == refList->TopFieldOrderCnt) {
                        ///check for complementary field
                        flags = refFrame->flags | refList->flags;
                        //If both TOP and BOTTOM are set, we'll clear those flags
                        if ((flags & VA_PICTURE_H264_TOP_FIELD) &&
                            (flags & VA_PICTURE_H264_BOTTOM_FIELD)) {
                            refFrame->flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
                        }
                        found = true;  //already in the DPB; will not add this one
                        break;
                    }
                }
                if (found == false) {
                    // add a new reference to the DPB
                    dpb->picture_id = findSurface(refList);
                    if (dpb->picture_id == VA_INVALID_SURFACE) {
                        if (mLastReference != NULL) {
                            dpb->picture_id = mLastReference->renderBuffer.surface;
                        } else {
                            ETRACE("Reference frame %d is missing. Stop updating references frames.", getPOC(refList));
                            return DECODE_NO_REFERENCE;
                        }
                    }
                    dpb->flags = refList->flags;
                    // if it's bottom field in dpb, there must have top field in DPB,
                    // so clear the bottom flag, or will confuse VED to address top field
                    if (dpb->flags & VA_PICTURE_H264_BOTTOM_FIELD)
                        dpb->flags &= (~VA_PICTURE_H264_BOTTOM_FIELD);
                    dpb->frame_idx = refList->frame_idx;
                    dpb->TopFieldOrderCnt = refList->TopFieldOrderCnt;
                    dpb->BottomFieldOrderCnt = refList->BottomFieldOrderCnt;
                    dpb++;
                    picParam->num_ref_frames++;
                }
            }
        }
    }
    return DECODE_SUCCESS;
}

void VideoDecoderAVC::removeReferenceFromDPB(VAPictureParameterBufferH264 *picParam) {
    // remove the current frame from DPB as it can't be decoded.
    if ((picParam->CurrPic.flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
        (picParam->CurrPic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)) {
        DecodedPictureBuffer *dpb = mDPBs[mToggleDPB];
        int32_t poc = getPOC(&(picParam->CurrPic));
        for (int32_t i = 0; i < DPB_SIZE; i++, dpb++) {
            if (poc == dpb->poc) {
                dpb->poc = (int32_t)POC_DEFAULT;
                if (dpb->surfaceBuffer) {
                    dpb->surfaceBuffer->asReferernce = false;
                }
                dpb->surfaceBuffer = NULL;
                break;
            }
        }
    }
}

int32_t VideoDecoderAVC::getPOC(VAPictureH264 *pic) {
    if (pic->flags & VA_PICTURE_H264_BOTTOM_FIELD) {
        return pic->BottomFieldOrderCnt;
    }
    return pic->TopFieldOrderCnt;
}

VASurfaceID VideoDecoderAVC::findSurface(VAPictureH264 *pic) {
    VideoSurfaceBuffer *p = findSurfaceBuffer(pic);
    if (p == NULL) {
        ETRACE("Could not find surface for poc %d", getPOC(pic));
        return VA_INVALID_SURFACE;
    }
    return p->renderBuffer.surface;
}

VideoSurfaceBuffer* VideoDecoderAVC::findSurfaceBuffer(VAPictureH264 *pic) {
    DecodedPictureBuffer *dpb = mDPBs[mToggleDPB];
    for (int32_t i = 0; i < DPB_SIZE; i++, dpb++) {
        if (dpb->poc == pic->BottomFieldOrderCnt ||
            dpb->poc == pic->TopFieldOrderCnt) {
            // TODO: remove these debugging codes
            if (dpb->surfaceBuffer == NULL) {
                ETRACE("Invalid surface buffer in the DPB for poc %d.", getPOC(pic));
            }
            return dpb->surfaceBuffer;
        }
    }
    // ETRACE("Unable to find surface for poc %d", getPOC(pic));
    return NULL;
}

VideoSurfaceBuffer* VideoDecoderAVC::findRefSurfaceBuffer(VAPictureH264 *pic) {
    DecodedPictureBuffer *dpb = mDPBs[mToggleDPB];
    // always looking for the latest one in the DPB, in case ref frames have same POC
    dpb += (DPB_SIZE - 1);
    for (int32_t i = DPB_SIZE; i > 0; i--, dpb--) {
        if (dpb->poc == pic->BottomFieldOrderCnt ||
            dpb->poc == pic->TopFieldOrderCnt) {
            // TODO: remove these debugging codes
            if (dpb->surfaceBuffer == NULL) {
                ETRACE("Invalid surface buffer in the DPB for poc %d.", getPOC(pic));
            }
            return dpb->surfaceBuffer;
        }
    }
    ETRACE("Unable to find surface for poc %d", getPOC(pic));
    return NULL;
}

void VideoDecoderAVC::invalidateDPB(int toggle) {
    DecodedPictureBuffer* p = mDPBs[toggle];
    for (int i = 0; i < DPB_SIZE; i++) {
        p->poc = (int32_t) POC_DEFAULT;
        p->surfaceBuffer = NULL;
        p++;
    }
}

void VideoDecoderAVC::clearAsReference(int toggle) {
    DecodedPictureBuffer* p = mDPBs[toggle];
    for (int i = 0; i < DPB_SIZE; i++) {
        if (p->surfaceBuffer) {
            p->surfaceBuffer->asReferernce = false;
        }
        p++;
    }
}

Decode_Status VideoDecoderAVC::startVA(vbp_data_h264 *data) {
    int32_t DPBSize = getDPBSize(data);

    //Use high profile for all kinds of H.264 profiles (baseline, main and high) except for constrained baseline
    VAProfile vaProfile = VAProfileH264High;

    // TODO: determine when to use VAProfileH264ConstrainedBaseline, set only if we are told to do so
    if ((data->codec_data->profile_idc == 66 || data->codec_data->constraint_set0_flag == 1) &&
        data->codec_data->constraint_set1_flag == 1) {
        if (mErrorConcealment) {
            vaProfile = VAProfileH264ConstrainedBaseline;
        }
    }

    VideoDecoderBase::setOutputWindowSize(mConfigBuffer.flag & WANT_ADAPTIVE_PLAYBACK ? OUTPUT_WINDOW_SIZE : DPBSize);
    updateFormatInfo(data);

   // for 1080p, limit the total surface to 19, according the hardware limitation
   // change the max surface number from 19->10 to workaround memory shortage
   // remove the workaround
    if(mVideoFormatInfo.height == 1088 && DPBSize + AVC_EXTRA_SURFACE_NUMBER > 19) {
        DPBSize = 19 - AVC_EXTRA_SURFACE_NUMBER;
    }

    if (mConfigBuffer.flag & WANT_ADAPTIVE_PLAYBACK) {
        // When Adaptive playback is enabled, turn off low delay mode.
        // Otherwise there may be a 240ms stuttering if the output mode is changed from LowDelay to Delay.
        enableLowDelayMode(false);
    } else {
        // for baseline profile, enable low delay mode automatically
        enableLowDelayMode(data->codec_data->profile_idc == 66);
    }

    return VideoDecoderBase::setupVA(DPBSize + AVC_EXTRA_SURFACE_NUMBER, vaProfile);
}

void VideoDecoderAVC::updateFormatInfo(vbp_data_h264 *data) {
    // new video size
    uint32_t width = (data->pic_data[0].pic_parms->picture_width_in_mbs_minus1 + 1) * 16;
    uint32_t height = (data->pic_data[0].pic_parms->picture_height_in_mbs_minus1 + 1) * 16;
    ITRACE("updateFormatInfo: current size: %d x %d, new size: %d x %d",
        mVideoFormatInfo.width, mVideoFormatInfo.height, width, height);

    if ((mVideoFormatInfo.width != width ||
        mVideoFormatInfo.height != height) &&
        width && height) {
        if (VideoDecoderBase::alignMB(mVideoFormatInfo.width) != width ||
            VideoDecoderBase::alignMB(mVideoFormatInfo.height) != height) {
            mSizeChanged = true;
            ITRACE("Video size is changed.");
        }
        mVideoFormatInfo.width = width;
        mVideoFormatInfo.height = height;
    }

    // video_range has default value of 0.
    mVideoFormatInfo.videoRange = data->codec_data->video_full_range_flag;

    switch (data->codec_data->matrix_coefficients) {
        case 1:
            mVideoFormatInfo.colorMatrix = VA_SRC_BT709;
            break;

        // ITU-R Recommendation BT.470-6 System B, G (MP4), same as
        // SMPTE 170M/BT601
        case 5:
        case 6:
            mVideoFormatInfo.colorMatrix = VA_SRC_BT601;
            break;

        default:
            // unknown color matrix, set to 0 so color space flag will not be set.
            mVideoFormatInfo.colorMatrix = 0;
            break;
    }
    mVideoFormatInfo.aspectX = data->codec_data->sar_width;
    mVideoFormatInfo.aspectY = data->codec_data->sar_height;
    mVideoFormatInfo.bitrate = data->codec_data->bit_rate;
    mVideoFormatInfo.cropLeft = data->codec_data->crop_left;
    mVideoFormatInfo.cropRight = data->codec_data->crop_right;
    mVideoFormatInfo.cropTop = data->codec_data->crop_top;
    mVideoFormatInfo.cropBottom = data->codec_data->crop_bottom;

    ITRACE("Cropping: left = %d, top = %d, right = %d, bottom = %d",
        data->codec_data->crop_left,
        data->codec_data->crop_top,
        data->codec_data->crop_right,
        data->codec_data->crop_bottom);

    if (mConfigBuffer.flag & WANT_SURFACE_PROTECTION) {
        mVideoFormatInfo.actualBufferNeeded = mConfigBuffer.surfaceNumber;
    } else {
        // The number of actual buffer needed is
        // outputQueue + nativewindow_owned + num_ref_frames + widi_need_max + 1(available buffer)
        // while outputQueue = DPB < 8? DPB :8
        mVideoFormatInfo.actualBufferNeeded = mOutputWindowSize + NW_CONSUMED /* Owned by native window */
                                              + data->codec_data->num_ref_frames
#ifndef USE_GEN_HW
                                              + HDMI_CONSUMED /* Two extra buffers are needed for native window buffer cycling */
                                              + (mWiDiOn ? WIDI_CONSUMED : 0) /* WiDi maximum needs */
#endif
                                              + 1;
    }

    ITRACE("actualBufferNeeded =%d", mVideoFormatInfo.actualBufferNeeded);

    mVideoFormatInfo.valid = true;

    setRenderRect();
}

bool VideoDecoderAVC::isWiDiStatusChanged() {
#ifndef USE_GEN_HW
    if (mWiDiOn)
        return false;

    if (mConfigBuffer.flag & WANT_SURFACE_PROTECTION)
        return false;

    if (!(mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER))
        return false;

    char prop[PROPERTY_VALUE_MAX];
    bool widi_on = (property_get("media.widi.enabled", prop, NULL) > 0) &&
                    (!strcmp(prop, "1") || !strcasecmp(prop, "true"));
    if (widi_on) {
        mVideoFormatInfo.actualBufferNeeded += WIDI_CONSUMED;
        mWiDiOn = true;
        ITRACE("WiDi is enabled, actual buffer needed is %d", mVideoFormatInfo.actualBufferNeeded);
        return true;
    }
    return false;
#else
    return false;
#endif
}

Decode_Status VideoDecoderAVC::handleNewSequence(vbp_data_h264 *data) {
    updateFormatInfo(data);
    bool needFlush = false;
    bool rawDataMode = !(mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER);

    if (!rawDataMode) {
        needFlush = (mVideoFormatInfo.width > mVideoFormatInfo.surfaceWidth)
                || (mVideoFormatInfo.height > mVideoFormatInfo.surfaceHeight)
                || isWiDiStatusChanged()
                || (mVideoFormatInfo.actualBufferNeeded > mConfigBuffer.surfaceNumber);
    }

    if (needFlush || (rawDataMode && mSizeChanged)) {
        mSizeChanged = false;
        flushSurfaceBuffers();
        return DECODE_FORMAT_CHANGE;
    } else
        return DECODE_SUCCESS;
}

bool VideoDecoderAVC::isNewFrame(vbp_data_h264 *data, bool equalPTS) {
    if (data->num_pictures == 0) {
        ETRACE("num_pictures == 0");
        return true;
    }

    vbp_picture_data_h264* picData = data->pic_data;
    if (picData->num_slices == 0) {
        ETRACE("num_slices == 0");
        return true;
    }

    bool newFrame = false;
    uint32_t fieldFlags = VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD;

    if (picData->slc_data[0].slc_parms.first_mb_in_slice != 0) {
        // not the first slice, assume it is continuation of a partial frame
        // TODO: check if it is new frame boundary as the first slice may get lost in streaming case.
        WTRACE("first_mb_in_slice != 0");
        if (!equalPTS) {
            // return true if different timestamp, it is a workaround here for a streaming case
            WTRACE("different PTS, treat it as a new frame");
            return true;
        }
    } else {
        if ((picData->pic_parms->CurrPic.flags & fieldFlags) == fieldFlags) {
            ETRACE("Current picture has both odd field and even field.");
        }
        // current picture is a field or a frame, and buffer conains the first slice, check if the current picture and
        // the last picture form an opposite field pair
        if (((mLastPictureFlags | picData->pic_parms->CurrPic.flags) & fieldFlags) == fieldFlags) {
            // opposite field
            newFrame = false;
            WTRACE("current picture is not at frame boundary.");
            mLastPictureFlags = 0;
        } else {
            newFrame = true;
            mLastPictureFlags = 0;
            for (uint32_t i = 0; i < data->num_pictures; i++) {
                mLastPictureFlags |= data->pic_data[i].pic_parms->CurrPic.flags;
            }
            if ((mLastPictureFlags & fieldFlags) == fieldFlags) {
                // current buffer contains both odd field and even field.
                mLastPictureFlags = 0;
            }
        }
    }
    
    return newFrame;
}

int32_t VideoDecoderAVC::getDPBSize(vbp_data_h264 *data) {
    // 1024 * MaxDPB / ( PicWidthInMbs * FrameHeightInMbs * 384 ), 16
    struct DPBTable {
        int32_t level;
        float maxDPB;
    } dpbTable[] = {
        {9,  148.5},
        {10, 148.5},
        {11, 337.5},
        {12, 891.0},
        {13, 891.0},
        {20, 891.0},
        {21, 1782.0},
        {22, 3037.5},
        {30, 3037.5},
        {31, 6750.0},
        {32, 7680.0},
        {40, 12288.0},
        {41, 12288.0},
        {42, 13056.0},
        {50, 41400.0},
        {51, 69120.0}
    };

    int32_t count = sizeof(dpbTable)/sizeof(DPBTable);
    float maxDPB = 0;
    for (int32_t i = 0; i < count; i++)
    {
        if (dpbTable[i].level == data->codec_data->level_idc) {
            maxDPB = dpbTable[i].maxDPB;
            break;
        }
    }

    int32_t maxDPBSize = maxDPB * 1024 / (
        (data->pic_data[0].pic_parms->picture_width_in_mbs_minus1 + 1) *
        (data->pic_data[0].pic_parms->picture_height_in_mbs_minus1 + 1) *
        384);

    if (maxDPBSize > 16) {
        maxDPBSize = 16;
    } else if (maxDPBSize == 0) {
        maxDPBSize = 3;
    }
    if(maxDPBSize < data->codec_data->num_ref_frames) {
        maxDPBSize = data->codec_data->num_ref_frames;
    }

    // add one extra frame for current frame.
    maxDPBSize += 1;
    ITRACE("maxDPBSize = %d, num_ref_frame = %d", maxDPBSize, data->codec_data->num_ref_frames);
    return maxDPBSize;
}

Decode_Status VideoDecoderAVC::checkHardwareCapability() {
#ifndef USE_GEN_HW
    VAStatus vaStatus;
    VAConfigAttrib cfgAttribs[2];
    cfgAttribs[0].type = VAConfigAttribMaxPictureWidth;
    cfgAttribs[1].type = VAConfigAttribMaxPictureHeight;
    vaStatus = vaGetConfigAttributes(mVADisplay, VAProfileH264High,
            VAEntrypointVLD, cfgAttribs, 2);
    CHECK_VA_STATUS("vaGetConfigAttributes");
    if (cfgAttribs[0].value * cfgAttribs[1].value < (uint32_t)mVideoFormatInfo.width * (uint32_t)mVideoFormatInfo.height) {
        ETRACE("hardware supports resolution %d * %d smaller than the clip resolution %d * %d",
                cfgAttribs[0].value, cfgAttribs[1].value, mVideoFormatInfo.width, mVideoFormatInfo.height);
        return DECODE_DRIVER_FAIL;
    }
#endif
    return DECODE_SUCCESS;
}

#ifdef USE_AVC_SHORT_FORMAT
Decode_Status VideoDecoderAVC::getCodecSpecificConfigs(
    VAProfile profile, VAConfigID *config)
{
    VAStatus vaStatus;
    VAConfigAttrib attrib[2];

    if (config == NULL) {
        ETRACE("Invalid parameter!");
        return DECODE_FAIL;
    }

    attrib[0].type = VAConfigAttribRTFormat;
    attrib[0].value = VA_RT_FORMAT_YUV420;
    attrib[1].type = VAConfigAttribDecSliceMode;
    attrib[1].value = VA_DEC_SLICE_MODE_NORMAL;

    vaStatus = vaGetConfigAttributes(mVADisplay,profile,VAEntrypointVLD, &attrib[1], 1);

    if (attrib[1].value & VA_DEC_SLICE_MODE_BASE) {
        ITRACE("AVC short format used");
        attrib[1].value = VA_DEC_SLICE_MODE_BASE;
    } else if (attrib[1].value & VA_DEC_SLICE_MODE_NORMAL) {
        ITRACE("AVC long format ssed");
        attrib[1].value = VA_DEC_SLICE_MODE_NORMAL;
    } else {
        ETRACE("Unsupported Decode Slice Mode!");
        return DECODE_FAIL;
    }

    vaStatus = vaCreateConfig(
            mVADisplay,
            profile,
            VAEntrypointVLD,
            &attrib[0],
            2,
            config);
    CHECK_VA_STATUS("vaCreateConfig");

    return DECODE_SUCCESS;
}
#endif
