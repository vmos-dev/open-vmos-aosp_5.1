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

#ifndef VIDEO_DECODER_VP8_H_
#define VIDEO_DECODER_VP8_H_

#include "VideoDecoderBase.h"


class VideoDecoderVP8 : public VideoDecoderBase {
public:
    VideoDecoderVP8(const char *mimeType);
    virtual ~VideoDecoderVP8();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

protected:
    virtual Decode_Status checkHardwareCapability();

private:
    Decode_Status decodeFrame(VideoDecodeBuffer* buffer, vbp_data_vp8 *data);
    Decode_Status decodePicture(vbp_data_vp8 *data, int32_t picIndex);
    Decode_Status setReference(VAPictureParameterBufferVP8 *picParam);
    Decode_Status startVA(vbp_data_vp8 *data);
    void updateReferenceFrames(vbp_data_vp8 *data);
    void refreshLastReference(vbp_data_vp8 *data);
    void refreshGoldenReference(vbp_data_vp8 *data);
    void refreshAltReference(vbp_data_vp8 *data);
    void updateFormatInfo(vbp_data_vp8 *data);
    void invalidateReferenceFrames(int toggle);
    void clearAsReference(int toggle, int ref_type);

private:
    enum {
        VP8_SURFACE_NUMBER = 9,
        VP8_REF_SIZE = 3,
    };

    enum {
        VP8_KEY_FRAME = 0,
        VP8_INTER_FRAME,
        VP8_SKIPPED_FRAME,
    };

    enum {
        VP8_LAST_REF_PIC = 0,
        VP8_GOLDEN_REF_PIC,
        VP8_ALT_REF_PIC,
    };

    enum {
        BufferCopied_NoneToGolden   = 0,
        BufferCopied_LastToGolden   = 1,
        BufferCopied_AltRefToGolden = 2
    };

    enum {
        BufferCopied_NoneToAltRef   = 0,
        BufferCopied_LastToAltRef   = 1,
        BufferCopied_GoldenToAltRef = 2
    };

    struct ReferenceFrameBuffer {
        VideoSurfaceBuffer *surfaceBuffer;
        int32_t index;
    };

    //[2] : [0 for current each reference frame, 1 for the previous each reference frame]
    //[VP8_REF_SIZE] : [0 for last ref pic, 1 for golden ref pic, 2 for alt ref pic]
    ReferenceFrameBuffer mRFBs[2][VP8_REF_SIZE];
};



#endif /* VIDEO_DECODER_VP8_H_ */
