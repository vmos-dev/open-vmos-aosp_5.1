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

#ifndef VIDEO_DECODER_WMV_H_
#define VIDEO_DECODER_WMV_H_

#include "VideoDecoderBase.h"


class VideoDecoderWMV : public VideoDecoderBase {
public:
    VideoDecoderWMV(const char *mimeType);
    virtual ~VideoDecoderWMV();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

protected:
    virtual Decode_Status checkHardwareCapability();


private:
    Decode_Status decodeFrame(VideoDecodeBuffer *buffer, vbp_data_vc1 *data);
    Decode_Status decodePicture(vbp_data_vc1 *data, int32_t picIndex);
    Decode_Status setReference(VAPictureParameterBufferVC1 *params, int32_t picIndex, VASurfaceID current);
    void updateDeblockedPicIndexes(int frameType);
    Decode_Status updateConfigData(uint8_t *configData, int32_t configDataLen, uint8_t **newConfigData, int32_t *newConfigDataLen);
    Decode_Status startVA(vbp_data_vc1 *data);
    void updateFormatInfo(vbp_data_vc1 *data);
    inline Decode_Status allocateVABufferIDs(int32_t number);
    Decode_Status parseBuffer(uint8_t *data, int32_t size, vbp_data_vc1 **vbpData);

private:
    enum {
        VC1_SURFACE_NUMBER = 10,
        VC1_EXTRA_SURFACE_NUMBER = 3,
    };

    VABufferID *mBufferIDs;
    int32_t mNumBufferIDs;
    bool mConfigDataParsed;
    bool mRangeMapped;

    int32_t mDeblockedCurrPicIndex;
    int32_t mDeblockedLastPicIndex;
    int32_t mDeblockedForwardPicIndex;
};



#endif /* VIDEO_DECODER_WMV_H_ */
