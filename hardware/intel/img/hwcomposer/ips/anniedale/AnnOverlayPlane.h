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
#ifndef ANN_OVERLAY_PLANE_H
#define ANN_OVERLAY_PLANE_H

#include <utils/KeyedVector.h>
#include <hal_public.h>
#include <DisplayPlane.h>
#include <BufferMapper.h>
#include <ips/common/Wsbm.h>
#include <ips/common/OverlayPlaneBase.h>
#include <ips/common/RotationBufferProvider.h>

namespace android {
namespace intel {

class AnnOverlayPlane : public OverlayPlaneBase {
public:
    AnnOverlayPlane(int index, int disp);
    virtual ~AnnOverlayPlane();

    bool setDataBuffer(uint32_t handle);

    virtual void setTransform(int transform);
    virtual void setZOrderConfig(ZOrderConfig& config, void *nativeConfig);

    // plane operations
    virtual bool flip(void *ctx);
    virtual bool reset();
    virtual bool enable();
    virtual bool disable();
    virtual void postFlip();
    virtual void* getContext() const;
    virtual bool initialize(uint32_t bufferCount);
    virtual void deinitialize();
    virtual bool rotatedBufferReady(BufferMapper& mapper, BufferMapper* &rotatedMapper);
    virtual bool useOverlayRotation(BufferMapper& mapper);

private:
    void signalVideoRotation(BufferMapper& mapper);
    bool isSettingRotBitAllowed();
protected:
    virtual bool setDataBuffer(BufferMapper& mapper);
    virtual bool flush(uint32_t flags);
    virtual bool bufferOffsetSetup(BufferMapper& mapper);
    virtual bool coordinateSetup(BufferMapper& mapper);
    virtual bool scalingSetup(BufferMapper& mapper);

    virtual void resetBackBuffer(int buf);

    RotationBufferProvider *mRotationBufProvider;

    // rotation config
    uint32_t mRotationConfig;
    // z order config
    uint32_t mZOrderConfig;
    bool mUseOverlayRotation;
    // hardware context
    struct intel_dc_plane_ctx mContext;

};

} // namespace intel
} // namespace android

#endif /* ANN_OVERLAY_PLANE_H */

