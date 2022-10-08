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

#ifndef __VIDEO_ENCODER_UTILS_H__
#define __VIDEO_ENCODER_UTILS_H__
#include <va/va.h>
#include <va/va_tpi.h>
#include "VideoEncoderDef.h"
#include "IntelMetadataBuffer.h"
#ifdef IMG_GFX
#include <hardware/gralloc.h>
#endif

#define MAP_ACTION_COPY         0x00000001  //mem copy
#define MAP_ACTION_ALIGN64      0x00000002  //align 64
#define MAP_ACTION_COLORCONVERT 0x00000004  //color convert
#define MAP_ACTION_RESIZE       0x00000008  //resize

class VASurfaceMap {
public:
    VASurfaceMap(VADisplay display, int hwcap);
    ~VASurfaceMap();

    Encode_Status doMapping();
    VASurfaceID getVASurface() {return mVASurface;}
    intptr_t getValue() {return mValue;}
    ValueInfo* getValueInfo() {return &mVinfo;}

    void setVASurface(VASurfaceID surface) {mVASurface = surface;}
    void setValue(intptr_t value) {mValue = value;}
    void setValueInfo(ValueInfo& vinfo) {memcpy(&mVinfo, &vinfo, sizeof(ValueInfo));}
    void setTracked() {mTracked = true;}
    void setAction(int32_t action) {mAction = action;}

private:
    Encode_Status doActionCopy();
    Encode_Status doActionColConv();
    Encode_Status MappingToVASurface();
    Encode_Status MappingSurfaceID(intptr_t value);
    Encode_Status MappingGfxHandle(intptr_t value);
    Encode_Status MappingKbufHandle(intptr_t value);
    Encode_Status MappingMallocPTR(intptr_t value);
    VASurfaceID CreateSurfaceFromExternalBuf(intptr_t value, ValueInfo& vinfo);

    VADisplay mVADisplay;

    intptr_t mValue;

    VASurfaceID mVASurface;
    int32_t mVASurfaceWidth;
    int32_t mVASurfaceHeight;
    int32_t mVASurfaceStride;

//    MetadataBufferType mType;

    ValueInfo mVinfo;
    bool mTracked;

    int32_t mAction;

    int32_t mSupportedSurfaceMemType;

#ifdef IMG_GFX
    //special for gfx color format converter
    buffer_handle_t mGfxHandle;
#endif
};

VASurfaceID CreateNewVASurface(VADisplay display, int32_t width, int32_t height);

#endif

