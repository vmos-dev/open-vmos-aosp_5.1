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

#ifndef VIDEO_DECODER_AVC_SECURE_H_
#define VIDEO_DECODER_AVC_SECURE_H_

#include "VideoDecoderAVC.h"
#include "secvideoparser.h"

class VideoDecoderAVCSecure : public VideoDecoderAVC {
public:
    VideoDecoderAVCSecure(const char *mimeType);
    virtual ~VideoDecoderAVCSecure();
    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

protected:
    virtual Decode_Status getCodecSpecificConfigs(VAProfile profile, VAConfigID*config);

private:
    virtual Decode_Status decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex);
private:
    pavp_info_t mEncParam;
    uint8_t *mNaluHeaderBuffer;
    uint8_t *mSliceHeaderBuffer;
    uint32_t mIsEncryptData;
    uint32_t mFrameSize;
};

#endif /* VIDEO_DECODER_AVC_SECURE_H_ */
