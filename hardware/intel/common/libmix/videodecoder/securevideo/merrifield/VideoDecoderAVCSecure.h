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

#ifndef VIDEO_DECODER_AVC_SECURE_H
#define VIDEO_DECODER_AVC_SECURE_H

#include "VideoDecoderBase.h"
#include "VideoDecoderAVC.h"
#include "VideoDecoderDefs.h"

class VideoDecoderAVCSecure : public VideoDecoderAVC {
public:
    VideoDecoderAVCSecure(const char *mimeType);
    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);

    // data in the decoded buffer is all encrypted.
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);
protected:
    virtual Decode_Status decodeFrame(VideoDecodeBuffer *buffer, vbp_data_h264 *data);
    virtual Decode_Status continueDecodingFrame(vbp_data_h264 *data);
    virtual Decode_Status beginDecodingFrame(vbp_data_h264 *data);
    virtual Decode_Status getCodecSpecificConfigs(VAProfile profile, VAConfigID*config);
    Decode_Status parseClassicSliceHeader(VideoDecodeBuffer *buffer, vbp_data_h264 *data);
    Decode_Status parseModularSliceHeader(VideoDecodeBuffer *buffer, vbp_data_h264 *data);

    Decode_Status updateSliceParameter(vbp_data_h264 *data, void *sliceheaderbuf);
    virtual Decode_Status decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex);
private:
    Decode_Status processClassicInputBuffer(VideoDecodeBuffer *buffer, vbp_data_h264 **data);
    Decode_Status processModularInputBuffer(VideoDecodeBuffer *buffer, vbp_data_h264 **data);
    int32_t     mIsEncryptData;
    int32_t     mFrameSize;
    uint8_t*    mFrameData;
    uint8_t*    mClearData;
    uint8_t*    mCachedHeader;
    int32_t     mFrameIdx;
    int32_t     mModularMode;

    enum {
        MAX_SLICE_HEADER_NUM  = 256,
    };
    int32_t     mSliceNum;
    // Information of Slices in the Modular DRM Mode
    struct SliceInfo {
        uint8_t     sliceHeaderByte;             //  first byte of the slice header
        uint32_t    sliceStartOffset;            // offset of Slice unit in the firewalled buffer
        uint32_t    sliceByteOffset;             // extra offset from the blockAligned slice offset
        uint32_t    sliceSize;                   // block aligned length of slice unit
        uint32_t    sliceLength;                 // actual size of the slice
    };

    SliceInfo mSliceInfo[MAX_SLICE_HEADER_NUM];
};

#endif
