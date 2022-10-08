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

#ifndef VIDEO_DECODER_MPEG4_H_
#define VIDEO_DECODER_MPEG4_H_

#include "VideoDecoderBase.h"


class VideoDecoderMPEG4 : public VideoDecoderBase {
public:
    VideoDecoderMPEG4(const char *mimeType);
    virtual ~VideoDecoderMPEG4();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

protected:
    virtual Decode_Status checkHardwareCapability();

private:
    Decode_Status decodeFrame(VideoDecodeBuffer *buffer, vbp_data_mp42 *data);
    Decode_Status beginDecodingFrame(vbp_data_mp42 *data);
    Decode_Status continueDecodingFrame(vbp_data_mp42 *data);
    Decode_Status decodeSlice(vbp_data_mp42 *data, vbp_picture_data_mp42 *picData);
    Decode_Status setReference(VAPictureParameterBufferMPEG4 *picParam);
    Decode_Status startVA(vbp_data_mp42 *data);
    void updateFormatInfo(vbp_data_mp42 *data);

private:
    // Value of VOP type defined here follows MP4 spec
    enum {
        MP4_VOP_TYPE_I = 0,
        MP4_VOP_TYPE_P = 1,
        MP4_VOP_TYPE_B = 2,
        MP4_VOP_TYPE_S = 3,
    };

    enum {
        MP4_SURFACE_NUMBER = 10,
    };

    uint64_t mLastVOPTimeIncrement;
    bool mExpectingNVOP; // indicate if future n-vop is a placeholder of a packed frame
    bool mSendIQMatrixBuf; // indicate if iq_matrix_buffer is sent to driver
    int32_t mLastVOPCodingType;
    bool mIsSyncFrame; // indicate if it is SyncFrame in container
    bool mIsShortHeader; // indicate if it is short header format
    VideoExtensionBuffer mExtensionBuffer;
    PackedFrameData mPackedFrame;
};



#endif /* VIDEO_DECODER_MPEG4_H_ */
