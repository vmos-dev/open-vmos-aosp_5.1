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

#include "VideoEncoderMP4.h"
#include "VideoEncoderH263.h"
#include "VideoEncoderAVC.h"
#include "VideoEncoderVP8.h"
#ifndef IMG_GFX
#include "PVSoftMPEG4Encoder.h"
#endif
#include "VideoEncoderHost.h"
#include <string.h>
#include <cutils/properties.h>
#include <wrs_omxil_core/log.h>

int32_t gLogLevel = 0;

IVideoEncoder *createVideoEncoder(const char *mimeType) {

    char logLevelProp[PROPERTY_VALUE_MAX];

    if (property_get("libmix.debug", logLevelProp, NULL)) {
        gLogLevel = atoi(logLevelProp);
        LOGD("Debug level is %d", gLogLevel);
    }

    if (mimeType == NULL) {
        LOGE("NULL mime type");
        return NULL;
    }

    if (strcasecmp(mimeType, "video/avc") == 0 ||
            strcasecmp(mimeType, "video/h264") == 0) {
        VideoEncoderAVC *p = new VideoEncoderAVC();
        return (IVideoEncoder *)p;
    } else if (strcasecmp(mimeType, "video/h263") == 0) {
#ifdef IMG_GFX
        VideoEncoderH263 *p = new VideoEncoderH263();
#else
        PVSoftMPEG4Encoder *p = new PVSoftMPEG4Encoder("OMX.google.h263.encoder");
#endif
        return (IVideoEncoder *)p;
    } else if (strcasecmp(mimeType, "video/mpeg4") == 0 ||
            strcasecmp(mimeType, "video/mp4v-es") == 0) {
#ifdef IMG_GFX
        VideoEncoderMP4 *p = new VideoEncoderMP4();
#else
        PVSoftMPEG4Encoder *p = new PVSoftMPEG4Encoder("OMX.google.mpeg4.encoder");
#endif
        return (IVideoEncoder *)p;
    } else if (strcasecmp(mimeType, "video/x-vnd.on2.vp8") == 0) {
        VideoEncoderVP8 *p = new VideoEncoderVP8();
        return (IVideoEncoder *)p;
    } else {
        LOGE ("Unknown mime type: %s", mimeType);
    }
    return NULL;
}

void releaseVideoEncoder(IVideoEncoder *p) {
    if (p) delete p;
}

