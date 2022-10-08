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

#include "VideoDecoderWMV.h"
#include "VideoDecoderMPEG4.h"
#include "VideoDecoderAVC.h"

#ifdef USE_INTEL_SECURE_AVC
#include "VideoDecoderAVCSecure.h"
#endif

#ifdef USE_HW_VP8
#include "VideoDecoderVP8.h"
#endif
#include "VideoDecoderHost.h"
#include "VideoDecoderTrace.h"
#include <string.h>

IVideoDecoder* createVideoDecoder(const char* mimeType) {
    if (mimeType == NULL) {
        ETRACE("NULL mime type.");
        return NULL;
    }

    if (strcasecmp(mimeType, "video/wmv") == 0 ||
        strcasecmp(mimeType, "video/vc1") == 0 ||
        strcasecmp(mimeType, "video/x-ms-wmv") == 0) {
        VideoDecoderWMV *p = new VideoDecoderWMV(mimeType);
        return (IVideoDecoder *)p;
    } else if (strcasecmp(mimeType, "video/avc") == 0 ||
               strcasecmp(mimeType, "video/h264") == 0) {
        VideoDecoderAVC *p = new VideoDecoderAVC(mimeType);
        return (IVideoDecoder *)p;
    } else if (strcasecmp(mimeType, "video/mp4v-es") == 0 ||
               strcasecmp(mimeType, "video/mpeg4") == 0 ||
               strcasecmp(mimeType, "video/h263") == 0 ||
               strcasecmp(mimeType, "video/3gpp") == 0) {
        VideoDecoderMPEG4 *p = new VideoDecoderMPEG4(mimeType);
        return (IVideoDecoder *)p;
    }
#ifdef USE_INTEL_SECURE_AVC
    else if (strcasecmp(mimeType, "video/avc-secure") == 0) {
        VideoDecoderAVC *p = new VideoDecoderAVCSecure(mimeType);
        return (IVideoDecoder *)p;
    }
#endif

#ifdef USE_HW_VP8
    else if (strcasecmp(mimeType, "video/vp8") == 0 ||
        strcasecmp(mimeType, "video/x-vnd.on2.vp8") == 0) {
        VideoDecoderVP8 *p = new VideoDecoderVP8(mimeType);
        return (IVideoDecoder *)p;
    }
#endif

    else {
        ETRACE("Unknown mime type: %s", mimeType);
    }
    return NULL;
}

void releaseVideoDecoder(IVideoDecoder* p) {
    if (p) {
        const VideoFormatInfo *info  = p->getFormatInfo();
        if (info && info->mimeType) {
            ITRACE("Deleting decoder for %s", info->mimeType);
        }
    }
    delete p;
}


