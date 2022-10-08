/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#ifndef __ROTATIONO_BUFFER_PROVIDER_H__
#define __ROTATIONO_BUFFER_PROVIDER_H__

#include <va/va.h>
#include <sys/time.h>
#include <va/va_tpi.h>
#include <va/va_vpp.h>
#include <ips/common/Wsbm.h>
#include <utils/Timers.h>
#include <utils/KeyedVector.h>
#include <va/va_android.h>
#include <ips/common/VideoPayloadBuffer.h>

namespace android {
namespace intel {

#define Display unsigned int
typedef void* VADisplay;
typedef int VAStatus;

class RotationBufferProvider {

public:
    RotationBufferProvider(Wsbm* wsbm);
    ~RotationBufferProvider();

    bool initialize();
    void deinitialize();
    void reset();
    bool setupRotationBuffer(VideoPayloadBuffer *payload, int transform);
    bool prepareBufferInfo(int, int, int, VideoPayloadBuffer *, void *);

private:
    void invalidateCaches();
    bool startVA(VideoPayloadBuffer *payload, int transform);
    void stopVA();
    bool isContextChanged(int width, int height, int transform);
    int transFromHalToVa(int transform);
    uint32_t createWsbmBuffer(int width, int height, void **buf);
    int getStride(bool isTarget, int width);
    bool createVaSurface(VideoPayloadBuffer *payload, int transform, bool isTarget);
    void freeVaSurfaces();
    inline uint32_t getMilliseconds();

private:
    enum {
        MAX_SURFACE_NUM = 4
    };

    Wsbm* mWsbm;

    bool mVaInitialized;
    VADisplay mVaDpy;
    VAConfigID mVaCfg;
    VAContextID mVaCtx;
    VABufferID mVaBufFilter;
    VASurfaceID mSourceSurface;
    Display mDisplay;

    // rotation config variables
    int mWidth;
    int mHeight;
    int mTransform;

    int mRotatedWidth;
    int mRotatedHeight;
    int mRotatedStride;

    int mTargetIndex;
    int mKhandles[MAX_SURFACE_NUM];
    VASurfaceID mRotatedSurfaces[MAX_SURFACE_NUM];
    void *mDrmBuf[MAX_SURFACE_NUM];

    enum {
        TTM_WRAPPER_COUNT = 10,
    };

    KeyedVector<uint64_t, void*> mTTMWrappers; /* userPt/wsbmBuffer  */

    int mBobDeinterlace;
};

} // name space intel
} // name space android

#endif
