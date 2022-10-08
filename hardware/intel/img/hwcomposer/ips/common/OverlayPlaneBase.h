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
#ifndef OVERLAY_PLANE_BASE_H
#define OVERLAY_PLANE_BASE_H

#include <utils/KeyedVector.h>
#include <DisplayPlane.h>
#include <BufferMapper.h>
#include <ips/common/Wsbm.h>
#include <ips/common/OverlayHardware.h>
#include <ips/common/VideoPayloadBuffer.h>

namespace android {
namespace intel {

typedef struct {
    OverlayBackBufferBlk *buf;
    uint32_t gttOffsetInPage;
    uint32_t bufObject;
} OverlayBackBuffer;

class OverlayPlaneBase : public DisplayPlane {
public:
    OverlayPlaneBase(int index, int disp);
    virtual ~OverlayPlaneBase();

    virtual void invalidateBufferCache();

    virtual bool assignToDevice(int disp);

    virtual void setZOrderConfig(ZOrderConfig& config, void *nativeConfig);

    // plane operations
    virtual bool flip(void *ctx) = 0;
    virtual bool reset();
    virtual bool enable();
    virtual bool disable();
    virtual bool isDisabled();

    virtual void* getContext() const = 0;
    virtual bool initialize(uint32_t bufferCount);
    virtual void deinitialize();

protected:
    // generic overlay register flush
    virtual bool flush(uint32_t flags) = 0;
    virtual bool setDataBuffer(BufferMapper& mapper);
    virtual bool bufferOffsetSetup(BufferMapper& mapper);
    virtual uint32_t calculateSWidthSW(uint32_t offset, uint32_t width);
    virtual bool coordinateSetup(BufferMapper& mapper);
    virtual bool setCoeffRegs(double *coeff, int mantSize,
                                 coeffPtr pCoeff, int pos);
    virtual void updateCoeff(int taps, double fCutoff,
                                bool isHoriz, bool isY,
                                coeffPtr pCoeff);
    virtual bool scalingSetup(BufferMapper& mapper);
    virtual void checkPosition(int& x, int& y, int& w, int& h);
    virtual void checkCrop(int& x, int& y, int& w, int& h, int coded_width, int coded_height);


protected:
    // back buffer operations
    virtual OverlayBackBuffer* createBackBuffer();
    virtual void deleteBackBuffer(int buf);
    virtual void resetBackBuffer(int buf);

    virtual BufferMapper* getTTMMapper(BufferMapper& grallocMapper, struct VideoPayloadBuffer *payload);
    virtual void  putTTMMapper(BufferMapper* mapper);
    virtual bool rotatedBufferReady(BufferMapper& mapper, BufferMapper* &rotatedMapper);
    virtual bool useOverlayRotation(BufferMapper& mapper);

private:
    inline bool isActiveTTMBuffer(BufferMapper *mapper);
    void updateActiveTTMBuffers(BufferMapper *mapper);
    void invalidateActiveTTMBuffers();
    void invalidateTTMBuffers();

protected:
    // flush flags
    enum {
        PLANE_ENABLE     = 0x00000001UL,
        PLANE_DISABLE    = 0x00000002UL,
        UPDATE_COEF      = 0x00000004UL,
    };

    enum {
        OVERLAY_BACK_BUFFER_COUNT = 3,
        MAX_ACTIVE_TTM_BUFFERS = 3,
        OVERLAY_DATA_BUFFER_COUNT = 20,
    };

    // TTM data buffers
    KeyedVector<uint64_t, BufferMapper*> mTTMBuffers;
    // latest TTM buffers
    Vector<BufferMapper*> mActiveTTMBuffers;

    // overlay back buffer
    OverlayBackBuffer *mBackBuffer[OVERLAY_BACK_BUFFER_COUNT];
    int mCurrent;
    // wsbm
    Wsbm *mWsbm;
    // pipe config
    uint32_t mPipeConfig;

    int mBobDeinterlace;
};

} // namespace intel
} // namespace android

#endif /* OVERLAY_PLANE_BASE_H */

