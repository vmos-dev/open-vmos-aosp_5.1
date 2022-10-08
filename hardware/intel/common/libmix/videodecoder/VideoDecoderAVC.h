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

#ifndef VIDEO_DECODER_AVC_H_
#define VIDEO_DECODER_AVC_H_

#include "VideoDecoderBase.h"


class VideoDecoderAVC : public VideoDecoderBase {
public:
    VideoDecoderAVC(const char *mimeType);
    virtual ~VideoDecoderAVC();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

protected:
    virtual Decode_Status decodeFrame(VideoDecodeBuffer *buffer, vbp_data_h264 *data);
    virtual Decode_Status beginDecodingFrame(vbp_data_h264 *data);
    virtual Decode_Status continueDecodingFrame(vbp_data_h264 *data);
    virtual Decode_Status decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex);
    Decode_Status setReference(VASliceParameterBufferH264 *sliceParam);
    Decode_Status updateDPB(VAPictureParameterBufferH264 *picParam);
    Decode_Status updateReferenceFrames(vbp_picture_data_h264 *picData);
    void removeReferenceFromDPB(VAPictureParameterBufferH264 *picParam);
    int32_t getPOC(VAPictureH264 *pic); // Picture Order Count
    inline VASurfaceID findSurface(VAPictureH264 *pic);
    inline VideoSurfaceBuffer* findSurfaceBuffer(VAPictureH264 *pic);
    inline VideoSurfaceBuffer* findRefSurfaceBuffer(VAPictureH264 *pic);
    inline void invalidateDPB(int toggle);
    inline void clearAsReference(int toggle);
    Decode_Status startVA(vbp_data_h264 *data);
    void updateFormatInfo(vbp_data_h264 *data);
    Decode_Status handleNewSequence(vbp_data_h264 *data);
    bool isNewFrame(vbp_data_h264 *data, bool equalPTS);
    int32_t getDPBSize(vbp_data_h264 *data);
    virtual Decode_Status checkHardwareCapability();
#ifdef USE_AVC_SHORT_FORMAT
    virtual Decode_Status getCodecSpecificConfigs(VAProfile profile, VAConfigID*config);
#endif
    bool isWiDiStatusChanged();

private:
    struct DecodedPictureBuffer {
        VideoSurfaceBuffer *surfaceBuffer;
        int32_t poc; // Picture Order Count
    };

    enum {
        AVC_EXTRA_SURFACE_NUMBER = 11,
        // maximum DPB (Decoded Picture Buffer) size
        MAX_REF_NUMBER = 16,
        DPB_SIZE = 17,         // DPB_SIZE = MAX_REF_NUMBER + 1,
        REF_LIST_SIZE = 32,
    };

    // maintain 2 ping-pong decoded picture buffers
    DecodedPictureBuffer mDPBs[2][DPB_SIZE];
    uint8_t mToggleDPB; // 0 or 1
    bool mErrorConcealment;
    uint32_t mLastPictureFlags;
    VideoExtensionBuffer mExtensionBuffer;
    PackedFrameData mPackedFrame;
};



#endif /* VIDEO_DECODER_AVC_H_ */
