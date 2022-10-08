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

#include <math.h>
#include <common/utils/HwcTrace.h>
#include <common/base/Drm.h>
#include <Hwcomposer.h>
#include <ips/anniedale/AnnOverlayPlane.h>
#include <ips/tangier/TngGrallocBuffer.h>
#include <khronos/openmax/OMX_IntelVideoExt.h>
#include <DisplayQuery.h>

namespace android {
namespace intel {

AnnOverlayPlane::AnnOverlayPlane(int index, int disp)
    : OverlayPlaneBase(index, disp),
      mRotationBufProvider(NULL),
      mRotationConfig(0),
      mZOrderConfig(0),
      mUseOverlayRotation(true)
{
    CTRACE();

    memset(&mContext, 0, sizeof(mContext));
}

AnnOverlayPlane::~AnnOverlayPlane()
{
    CTRACE();
}

bool AnnOverlayPlane::setDataBuffer(uint32_t handle)
{
    if (handle == 0) {
        ELOGTRACE("handle == 0");
        return true;
    }

    return DisplayPlane::setDataBuffer(handle);
}

void AnnOverlayPlane::setZOrderConfig(ZOrderConfig& /* zorderConfig */,
        void *nativeConfig)
{
    int slot = (int)nativeConfig;

    CTRACE();

    switch (slot) {
    case 0:
        mZOrderConfig = 0;
        break;
    case 1:
        mZOrderConfig = (1 << 8);
        break;
    case 2:
        mZOrderConfig = (2 << 8);
        break;
    case 3:
        mZOrderConfig = (3 << 8);
        break;
    default:
        ELOGTRACE("Invalid overlay plane zorder %d", slot);
        return;
    }
}

bool AnnOverlayPlane::reset()
{
    OverlayPlaneBase::reset();
    if (mRotationBufProvider) {
        mRotationBufProvider->reset();
    }
    return true;
}

bool AnnOverlayPlane::enable()
{
    RETURN_FALSE_IF_NOT_INIT();

    // by default always use overlay rotation
    mUseOverlayRotation = true;

    if (mContext.ctx.ov_ctx.ovadd & (0x1 << 15))
        return true;

    mContext.ctx.ov_ctx.ovadd |= (0x1 << 15);

    // flush
    flush(PLANE_ENABLE);

    return true;
}

bool AnnOverlayPlane::disable()
{
    RETURN_FALSE_IF_NOT_INIT();

    if (!(mContext.ctx.ov_ctx.ovadd & (0x1 << 15)))
        return true;

    mContext.ctx.ov_ctx.ovadd &= ~(0x1 << 15);

    mContext.ctx.ov_ctx.ovadd &= ~(0x300);

    mContext.ctx.ov_ctx.ovadd |= mPipeConfig;

    // flush
    flush(PLANE_DISABLE);

    return true;
}

void AnnOverlayPlane::postFlip()
{
    // when using AnnOverlayPlane through AnnDisplayPlane as proxy, postFlip is never
    // called so mUpdateMasks is never reset.
    // When using AnnOverlayPlane directly, postFlip is invoked and mUpdateMasks is reset
    // post-flip.

    // need to check why mUpdateMasks = 0 causes video freeze.

    //DisplayPlane::postFlip();
}


void AnnOverlayPlane::resetBackBuffer(int buf)
{
    CTRACE();

    if (!mBackBuffer[buf] || !mBackBuffer[buf]->buf)
        return;

    OverlayBackBufferBlk *backBuffer = mBackBuffer[buf]->buf;

    memset(backBuffer, 0, sizeof(OverlayBackBufferBlk));

    // reset overlay
    backBuffer->OCLRC0 = (OVERLAY_INIT_CONTRAST << 18) |
                         (OVERLAY_INIT_BRIGHTNESS & 0xff);
    backBuffer->OCLRC1 = OVERLAY_INIT_SATURATION;
    backBuffer->DCLRKV = OVERLAY_INIT_COLORKEY;
    backBuffer->DCLRKM = OVERLAY_INIT_COLORKEYMASK;
    backBuffer->OCONFIG = 0;
    backBuffer->OCONFIG |= (0x1 << 27);
    // use 3 line buffers
    backBuffer->OCONFIG |= 0x1;
    backBuffer->SCHRKEN &= ~(0x7 << 24);
    backBuffer->SCHRKEN |= 0xff;
}

bool AnnOverlayPlane::bufferOffsetSetup(BufferMapper& mapper)
{
    CTRACE();

    OverlayBackBufferBlk *backBuffer = mBackBuffer[mCurrent]->buf;
    if (!backBuffer) {
        ELOGTRACE("invalid back buffer");
        return false;
    }

    uint32_t format = mapper.getFormat();
    uint32_t gttOffsetInBytes = (mapper.getGttOffsetInPage(0) << 12);

    if (format == HAL_PIXEL_FORMAT_BGRX_8888 ||
        format == HAL_PIXEL_FORMAT_BGRA_8888) {
        backBuffer->OCMD = 1 << 10;
        // by pass YUV->RGB conversion, 8-bit output
        backBuffer->OCONFIG |= (0x1 << 4) | (0x1 << 3);
        backBuffer->OSTART_0Y = gttOffsetInBytes;
        backBuffer->OSTART_1Y = gttOffsetInBytes;
        backBuffer->OBUF_0Y = 0;
        backBuffer->OBUF_1Y = 0;
        return true;
    }

    uint32_t yStride = mapper.getStride().yuv.yStride;
    uint32_t uvStride = mapper.getStride().yuv.uvStride;
    uint32_t w = mapper.getWidth();
    uint32_t h = mapper.getHeight();
    uint32_t srcX= mapper.getCrop().x;
    uint32_t srcY= mapper.getCrop().y;
    uint32_t ySurface, uSurface, vSurface;
    uint32_t yTileOffsetX, yTileOffsetY;
    uint32_t uTileOffsetX, uTileOffsetY;
    uint32_t vTileOffsetX, vTileOffsetY;

    // clear original format setting
    backBuffer->OCMD &= ~(0xf << 10);
    backBuffer->OCMD &= ~OVERLAY_MEMORY_LAYOUT_TILED;

    backBuffer->OBUF_0Y = 0;
    backBuffer->OBUF_0V = 0;
    backBuffer->OBUF_0U = 0;
    // Y/U/V plane must be 4k bytes aligned.
    ySurface = gttOffsetInBytes;
    if (mIsProtectedBuffer) {
        // temporary workaround until vsync event logic is corrected.
        // it seems that overlay buffer update and renderring can be overlapped,
        // as such encryption bit may be cleared during HW rendering
        ySurface |= 0x01;
    }

    switch(format) {
    case HAL_PIXEL_FORMAT_YV12:    // YV12
        vSurface = ySurface + yStride * h;
        uSurface = vSurface + uvStride * (h / 2);
        yTileOffsetX = srcX;
        yTileOffsetY = srcY;
        uTileOffsetX = srcX / 2;
        uTileOffsetY = srcY / 2;
        vTileOffsetX = uTileOffsetX;
        vTileOffsetY = uTileOffsetY;
        backBuffer->OCMD |= OVERLAY_FORMAT_PLANAR_YUV420;
        break;
    case HAL_PIXEL_FORMAT_I420:    // I420
        uSurface = ySurface + yStride * h;
        vSurface = uSurface + uvStride * (h / 2);
        yTileOffsetX = srcX;
        yTileOffsetY = srcY;
        uTileOffsetX = srcX / 2;
        uTileOffsetY = srcY / 2;
        vTileOffsetX = uTileOffsetX;
        vTileOffsetY = uTileOffsetY;
        backBuffer->OCMD |= OVERLAY_FORMAT_PLANAR_YUV420;
        break;
    case HAL_PIXEL_FORMAT_NV12:    // NV12
        uSurface = ySurface;
        vSurface = ySurface;
        backBuffer->OBUF_0U = yStride * h;
        yTileOffsetX = srcX;
        yTileOffsetY = srcY;
        uTileOffsetX = srcX / 2;
        uTileOffsetY = srcY / 2 + h;
        vTileOffsetX = uTileOffsetX;
        vTileOffsetY = uTileOffsetY;
        backBuffer->OCMD |= OVERLAY_FORMAT_PLANAR_NV12_2;
        break;
    // NOTE: this is the decoded video format, align the height to 32B
    //as it's defined by video driver
    case OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar:    // NV12
        uSurface = ySurface + yStride * align_to(h, 32);
        vSurface = ySurface + yStride * align_to(h, 32);
        yTileOffsetX = srcX;
        yTileOffsetY = srcY;
        uTileOffsetX = srcX;
        uTileOffsetY = srcY / 2;
        vTileOffsetX = uTileOffsetX;
        vTileOffsetY = uTileOffsetY;
        backBuffer->OCMD |= OVERLAY_FORMAT_PLANAR_NV12_2;
        break;
    case OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar_Tiled:  //NV12_tiled
        uSurface = ySurface + yStride * align_to(h, 32);
        vSurface = ySurface + yStride * align_to(h, 32);
        yTileOffsetX = srcX;
        yTileOffsetY = srcY;
        uTileOffsetX = srcX;
        uTileOffsetY = srcY / 2;
        vTileOffsetX = uTileOffsetX;
        vTileOffsetY = uTileOffsetY;
        backBuffer->OCMD |= OVERLAY_FORMAT_PLANAR_NV12_2;
        backBuffer->OCMD |= OVERLAY_MEMORY_LAYOUT_TILED;
        break;
    case HAL_PIXEL_FORMAT_YUY2:    // YUY2
        uSurface = ySurface;
        vSurface = ySurface;
        yTileOffsetX = srcX;
        yTileOffsetY = srcY;
        uTileOffsetX = yTileOffsetX;
        uTileOffsetY = yTileOffsetY;
        vTileOffsetX = yTileOffsetX;
        vTileOffsetY = yTileOffsetY;
        backBuffer->OCMD |= OVERLAY_FORMAT_PACKED_YUV422;
        backBuffer->OCMD |= OVERLAY_PACKED_ORDER_YUY2;
        break;
    case HAL_PIXEL_FORMAT_UYVY:    // UYVY
        uSurface = ySurface;
        vSurface = ySurface;
        yTileOffsetX = srcX;
        yTileOffsetY = srcY;
        uTileOffsetX = yTileOffsetX;
        uTileOffsetY = yTileOffsetY;
        vTileOffsetX = yTileOffsetX;
        vTileOffsetY = yTileOffsetY;
        backBuffer->OCMD |= OVERLAY_FORMAT_PACKED_YUV422;
        backBuffer->OCMD |= OVERLAY_PACKED_ORDER_UYVY;
        break;
    default:
        ELOGTRACE("unsupported format %d", format);
        return false;
    }

    backBuffer->OSTART_0Y = ySurface;
    backBuffer->OSTART_0U = uSurface;
    backBuffer->OSTART_0V = vSurface;
    backBuffer->OBUF_0Y += srcY * yStride + srcX;
    backBuffer->OBUF_0V += (srcY / 2) * uvStride + srcX;
    backBuffer->OBUF_0U += (srcY / 2) * uvStride + srcX;
    backBuffer->OTILEOFF_0Y = yTileOffsetY << 16 | yTileOffsetX;
    backBuffer->OTILEOFF_0U = uTileOffsetY << 16 | uTileOffsetX;
    backBuffer->OTILEOFF_0V = vTileOffsetY << 16 | vTileOffsetX;

    VLOGTRACE("done. offset (%d, %d, %d)",
          backBuffer->OBUF_0Y,
          backBuffer->OBUF_0U,
          backBuffer->OBUF_0V);

    return true;
}

bool AnnOverlayPlane::coordinateSetup(BufferMapper& mapper)
{
    CTRACE();

    uint32_t format = mapper.getFormat();
    if (format != HAL_PIXEL_FORMAT_BGRX_8888 &&
        format != HAL_PIXEL_FORMAT_BGRA_8888) {
        return OverlayPlaneBase::coordinateSetup(mapper);
    }

    OverlayBackBufferBlk *backBuffer = mBackBuffer[mCurrent]->buf;
    if (!backBuffer) {
        ELOGTRACE("invalid back buffer");
        return false;
    }

    backBuffer->SWIDTH = mapper.getCrop().w;
    backBuffer->SHEIGHT = mapper.getCrop().h;
    backBuffer->SWIDTHSW = calculateSWidthSW(0, mapper.getCrop().w) << 2;
    backBuffer->OSTRIDE = mapper.getStride().rgb.stride & (~0x3f);
    return true;
};

bool AnnOverlayPlane::scalingSetup(BufferMapper& mapper)
{
    int xscaleInt, xscaleFract, yscaleInt, yscaleFract;
    int xscaleIntUV, xscaleFractUV;
    int yscaleIntUV, yscaleFractUV;
    // UV is half the size of Y -- YUV420
    int uvratio = 2;
    uint32_t newval;
    coeffRec xcoeffY[N_HORIZ_Y_TAPS * N_PHASES];
    coeffRec xcoeffUV[N_HORIZ_UV_TAPS * N_PHASES];
    coeffRec ycoeffY[N_VERT_Y_TAPS * N_PHASES];
    coeffRec ycoeffUV[N_VERT_UV_TAPS * N_PHASES];
    int i, j, pos;
    bool scaleChanged = false;
    int x, y, w, h;
    int deinterlace_factor = 1;
    drmModeModeInfoPtr mode = &mModeInfo;

    OverlayBackBufferBlk *backBuffer = mBackBuffer[mCurrent]->buf;
    if (!backBuffer) {
        ELOGTRACE("invalid back buffer");
        return false;
    }

    if (mPanelOrientation == PANEL_ORIENTATION_180) {
        if (mode->hdisplay)
            x = mode->hdisplay - mPosition.x - mPosition.w;
        else
            x = mPosition.x;
        if (mode->vdisplay)
            y = mode->vdisplay - mPosition.y - mPosition.h;
        else
            y = mPosition.y;
    } else {
        x = mPosition.x;
        y = mPosition.y;
    }

    w = mPosition.w;
    h = mPosition.h;

    // check position
    checkPosition(x, y, w, h);
    VLOGTRACE("final position (%d, %d, %d, %d)", x, y, w, h);

    if ((w <= 0) || (h <= 0)) {
         ELOGTRACE("invalid dst width/height");
         return false;
    }

    // setup dst position
    backBuffer->DWINPOS = (y << 16) | x;
    backBuffer->DWINSZ = (h << 16) | w;

    uint32_t srcWidth = mapper.getCrop().w;
    uint32_t srcHeight = mapper.getCrop().h;
    uint32_t dstWidth = w;
    uint32_t dstHeight = h;
    uint32_t format = mapper.getFormat();

    if (format == HAL_PIXEL_FORMAT_BGRX_8888 ||
        format == HAL_PIXEL_FORMAT_BGRA_8888) {
        backBuffer->YRGBSCALE = 1 << 15 | 0 << 3 || 0 << 20;
        backBuffer->UVSCALEV = (1 << 16);
        return true;
    }

    if (mBobDeinterlace && !mTransform)
        deinterlace_factor = 2;

    VLOGTRACE("src (%dx%d), dst (%dx%d), transform %d",
          srcWidth, srcHeight,
          dstWidth, dstHeight,
          mTransform);

    // switch destination width/height for scale factor calculation
    // for 90/270 transformation
    if (mUseOverlayRotation && ((mTransform == HWC_TRANSFORM_ROT_90) ||
        (mTransform == HWC_TRANSFORM_ROT_270))) {
        uint32_t tmp = srcHeight;
        srcHeight = srcWidth;
        srcWidth = tmp;
    }

     // Y down-scale factor as a multiple of 4096
    if (srcWidth == dstWidth && srcHeight == dstHeight) {
        xscaleFract = (1 << 12);
        yscaleFract = (1 << 12) / deinterlace_factor;
    } else {
        xscaleFract = ((srcWidth - 1) << 12) / dstWidth;
        yscaleFract = ((srcHeight - 1) << 12) / (dstHeight * deinterlace_factor);
    }

    // Calculate the UV scaling factor
    xscaleFractUV = xscaleFract / uvratio;
    yscaleFractUV = yscaleFract / uvratio;


    // To keep the relative Y and UV ratios exact, round the Y scales
    // to a multiple of the Y/UV ratio.
    xscaleFract = xscaleFractUV * uvratio;
    yscaleFract = yscaleFractUV * uvratio;

    // Integer (un-multiplied) values
    xscaleInt = xscaleFract >> 12;
    yscaleInt = yscaleFract >> 12;

    xscaleIntUV = xscaleFractUV >> 12;
    yscaleIntUV = yscaleFractUV >> 12;

    // Check scaling ratio
    if (xscaleInt > INTEL_OVERLAY_MAX_SCALING_RATIO) {
        ELOGTRACE("xscaleInt > %d", INTEL_OVERLAY_MAX_SCALING_RATIO);
        return false;
    }

    // shouldn't get here
    if (xscaleIntUV > INTEL_OVERLAY_MAX_SCALING_RATIO) {
        ELOGTRACE("xscaleIntUV > %d", INTEL_OVERLAY_MAX_SCALING_RATIO);
        return false;
    }

    newval = (xscaleInt << 15) |
    ((xscaleFract & 0xFFF) << 3) | ((yscaleFract & 0xFFF) << 20);
    if (newval != backBuffer->YRGBSCALE) {
        scaleChanged = true;
        backBuffer->YRGBSCALE = newval;
    }

    newval = (xscaleIntUV << 15) | ((xscaleFractUV & 0xFFF) << 3) |
    ((yscaleFractUV & 0xFFF) << 20);
    if (newval != backBuffer->UVSCALE) {
        scaleChanged = true;
        backBuffer->UVSCALE = newval;
    }

    newval = yscaleInt << 16 | yscaleIntUV;
    if (newval != backBuffer->UVSCALEV) {
        scaleChanged = true;
        backBuffer->UVSCALEV = newval;
    }

    // Recalculate coefficients if the scaling changed
    // Only Horizontal coefficients so far.
    if (scaleChanged) {
        double fHCutoffY;
        double fHCutoffUV;
        double fVCutoffY;
        double fVCutoffUV;

        fHCutoffY = xscaleFract / 4096.0;
        fHCutoffUV = xscaleFractUV / 4096.0;
        fVCutoffY = yscaleFract / 4096.0;
        fVCutoffUV = yscaleFractUV / 4096.0;

        // Limit to between 1.0 and 3.0
        if (fHCutoffY < MIN_CUTOFF_FREQ)
            fHCutoffY = MIN_CUTOFF_FREQ;
        if (fHCutoffY > MAX_CUTOFF_FREQ)
            fHCutoffY = MAX_CUTOFF_FREQ;
        if (fHCutoffUV < MIN_CUTOFF_FREQ)
            fHCutoffUV = MIN_CUTOFF_FREQ;
        if (fHCutoffUV > MAX_CUTOFF_FREQ)
            fHCutoffUV = MAX_CUTOFF_FREQ;

        if (fVCutoffY < MIN_CUTOFF_FREQ)
            fVCutoffY = MIN_CUTOFF_FREQ;
        if (fVCutoffY > MAX_CUTOFF_FREQ)
            fVCutoffY = MAX_CUTOFF_FREQ;
        if (fVCutoffUV < MIN_CUTOFF_FREQ)
            fVCutoffUV = MIN_CUTOFF_FREQ;
        if (fVCutoffUV > MAX_CUTOFF_FREQ)
            fVCutoffUV = MAX_CUTOFF_FREQ;

        updateCoeff(N_HORIZ_Y_TAPS, fHCutoffY, true, true, xcoeffY);
        updateCoeff(N_HORIZ_UV_TAPS, fHCutoffUV, true, false, xcoeffUV);
        updateCoeff(N_VERT_Y_TAPS, fVCutoffY, false, true, ycoeffY);
        updateCoeff(N_VERT_UV_TAPS, fVCutoffUV, false, false, ycoeffUV);

        for (i = 0; i < N_PHASES; i++) {
            for (j = 0; j < N_HORIZ_Y_TAPS; j++) {
                pos = i * N_HORIZ_Y_TAPS + j;
                backBuffer->Y_HCOEFS[pos] =
                        (xcoeffY[pos].sign << 15 |
                         xcoeffY[pos].exponent << 12 |
                         xcoeffY[pos].mantissa);
            }
        }
        for (i = 0; i < N_PHASES; i++) {
            for (j = 0; j < N_HORIZ_UV_TAPS; j++) {
                pos = i * N_HORIZ_UV_TAPS + j;
                backBuffer->UV_HCOEFS[pos] =
                         (xcoeffUV[pos].sign << 15 |
                          xcoeffUV[pos].exponent << 12 |
                          xcoeffUV[pos].mantissa);
            }
        }

        for (i = 0; i < N_PHASES; i++) {
            for (j = 0; j < N_VERT_Y_TAPS; j++) {
                pos = i * N_VERT_Y_TAPS + j;
                backBuffer->Y_VCOEFS[pos] =
                        (ycoeffY[pos].sign << 15 |
                         ycoeffY[pos].exponent << 12 |
                         ycoeffY[pos].mantissa);
            }
        }
        for (i = 0; i < N_PHASES; i++) {
            for (j = 0; j < N_VERT_UV_TAPS; j++) {
                pos = i * N_VERT_UV_TAPS + j;
                backBuffer->UV_VCOEFS[pos] =
                         (ycoeffUV[pos].sign << 15 |
                          ycoeffUV[pos].exponent << 12 |
                          ycoeffUV[pos].mantissa);
            }
        }
    }

    XLOGTRACE();
    return true;
}

void AnnOverlayPlane::setTransform(int transform)
{
    RETURN_VOID_IF_NOT_INIT();

    if (mPanelOrientation == PANEL_ORIENTATION_180)
       transform ^=  HWC_TRANSFORM_ROT_180;

    DisplayPlane::setTransform(transform);

    // setup transform config
    switch (mTransform) {
    case HWC_TRANSFORM_ROT_90:
        mRotationConfig = (0x1 << 10);
        break;
    case HWC_TRANSFORM_ROT_180:
        mRotationConfig = (0x2 << 10);
        break;
    case HWC_TRANSFORM_ROT_270:
        mRotationConfig = (0x3 << 10);
        break;
    case 0:
        mRotationConfig = 0;
        break;
    default:
        ELOGTRACE("Invalid transform %d", mTransform);
        mRotationConfig = 0;
        break;
    }
}

// HSD 4645510:
// This is a SOC limition, that when source buffer width range is
// in (960, 1024] - one cache line length, and rotation bit is set
// in portrait mode, video will show distortion.
bool AnnOverlayPlane::isSettingRotBitAllowed()
{
    uint32_t width = mSrcCrop.w;

    if ((width > 960 && width <= 1024) &&
            (mTransform == 0 || mTransform == HAL_TRANSFORM_ROT_180))
        return false;
    return true;
}

bool AnnOverlayPlane::flip(void *ctx)
{
    uint32_t ovadd = 0;

    RETURN_FALSE_IF_NOT_INIT();

    if (!DisplayPlane::flip(ctx)) {
        ELOGTRACE("failed to flip display plane.");
        return false;
    }

    // update back buffer address
    ovadd = (mBackBuffer[mCurrent]->gttOffsetInPage << 12);

    // enable rotation mode and setup rotation config
    if (mIndex == 0 && mRotationConfig != 0) {
        if (isSettingRotBitAllowed())
            ovadd |= (1 << 12);
        ovadd |= mRotationConfig;
    }

    // setup z-order config
    ovadd |= mZOrderConfig;

    // load coefficients
    ovadd |= 0x1;

    // enable overlay
    ovadd |= (1 << 15);

    mContext.type = DC_OVERLAY_PLANE;
    mContext.ctx.ov_ctx.ovadd = ovadd;
    mContext.ctx.ov_ctx.index = mIndex;
    mContext.ctx.ov_ctx.pipe = mDevice;
    mContext.ctx.ov_ctx.ovadd |= mPipeConfig;

    // move to next back buffer
    mCurrent = (mCurrent + 1) % OVERLAY_BACK_BUFFER_COUNT;

    VLOGTRACE("ovadd = %#x, index = %d, device = %d",
          mContext.ctx.ov_ctx.ovadd,
          mIndex,
          mDevice);

    return true;
}

void* AnnOverlayPlane::getContext() const
{
    CTRACE();
    return (void *)&mContext;
}

bool AnnOverlayPlane::setDataBuffer(BufferMapper& mapper)
{
    if (mIsProtectedBuffer) {
        // workaround overlay scaling limitation
        float scaleX = (float)mSrcCrop.w/mPosition.w;
        float scaleY = (float)mSrcCrop.h/mPosition.h;
        if (scaleX > 4.0) {
            int crop = (mSrcCrop.w - 4 * mPosition.w)/2 + 1;
            mSrcCrop.x += crop;
            mSrcCrop.w -= 2 * crop;
        }

        if (scaleY > 4.0) {
            int crop = (mSrcCrop.h - 4 * mPosition.h)/2 + 1;
            mSrcCrop.y += crop;
            mSrcCrop.h -= 2 * crop;
        }

        if (scaleX > 4.0 || scaleY > 4.0) {
            mUpdateMasks |= PLANE_SOURCE_CROP_CHANGED;
            mapper.setCrop(mSrcCrop.x, mSrcCrop.y, mSrcCrop.w, mSrcCrop.h);
        }
    }


    if (OverlayPlaneBase::setDataBuffer(mapper) == false) {
        return false;
    }

    signalVideoRotation(mapper);

    if (mIsProtectedBuffer) {
        // Bit 0: Decryption request, only allowed to change on a synchronous flip
        // This request will be qualified with the separate decryption enable bit for OV
        mBackBuffer[mCurrent]->buf->OSTART_0Y |= 0x1;
        mBackBuffer[mCurrent]->buf->OSTART_1Y |= 0x1;
    }
    return true;
}

bool AnnOverlayPlane::initialize(uint32_t bufferCount)
{
    if (!OverlayPlaneBase::initialize(bufferCount)) {
        ELOGTRACE("failed to initialize OverlayPlaneBase");
        return false;
    }

    // setup rotation buffer
    mRotationBufProvider = new RotationBufferProvider(mWsbm);
    if (!mRotationBufProvider || !mRotationBufProvider->initialize()) {
        DEINIT_AND_RETURN_FALSE("failed to initialize RotationBufferProvider");
    }
    return true;
}

void AnnOverlayPlane::deinitialize()
{
    DEINIT_AND_DELETE_OBJ(mRotationBufProvider);
    OverlayPlaneBase::deinitialize();
}

bool AnnOverlayPlane::rotatedBufferReady(BufferMapper& mapper, BufferMapper* &rotatedMapper)
{
    struct VideoPayloadBuffer *payload;
    uint32_t format;
    // only NV12_VED has rotated buffer
    format = mapper.getFormat();
    if (format != OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar &&
        format != OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar_Tiled) {
        ELOGTRACE("invalid video format %#x", format);
        return false;
    }

    payload = (struct VideoPayloadBuffer *)mapper.getCpuAddress(SUB_BUFFER1);
    // check payload
    if (!payload) {
        ELOGTRACE("no payload found");
        return false;
    }

    if (payload->force_output_method == FORCE_OUTPUT_GPU) {
        ELOGTRACE("Output method is not supported!");
        return false;
    }

    if (payload->client_transform != mTransform ||
        mBobDeinterlace) {
        if (!mRotationBufProvider->setupRotationBuffer(payload, mTransform)) {
            DLOGTRACE("failed to setup rotation buffer");
            return false;
        }
    }

    rotatedMapper = getTTMMapper(mapper, payload);
    return true;
}

void AnnOverlayPlane::signalVideoRotation(BufferMapper& mapper)
{
    struct VideoPayloadBuffer *payload;
    uint32_t format;

    // check if it's video layer
    format = mapper.getFormat();
    if (format != OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar &&
        format != OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar_Tiled) {
        return;
    }

    payload = (struct VideoPayloadBuffer *)mapper.getCpuAddress(SUB_BUFFER1);
    if (!payload) {
        ELOGTRACE("no payload found");
        return;
    }

    /* if use overlay rotation, signal decoder to stop rotation */
    if (mUseOverlayRotation) {
        if (payload->client_transform) {
            WLOGTRACE("signal decoder to stop generate rotation buffer");
            payload->hwc_timestamp = systemTime();
            payload->layer_transform = 0;
        }
    } else {
        /* if overlay rotation cannot be used, signal decoder to start rotation */
        if (payload->client_transform != mTransform) {
            WLOGTRACE("signal decoder to generate rotation buffer with transform %d", mTransform);
            payload->hwc_timestamp = systemTime();
            payload->layer_transform = mTransform;
        }
    }
}

bool AnnOverlayPlane::useOverlayRotation(BufferMapper& /* mapper */)
{
    if (mTransform == 0)
        return true;

    if (!isSettingRotBitAllowed()) {
        mUseOverlayRotation = false;
        mRotationConfig = 0;
        return false;
    }

    // workaround limitation of overlay rotation by falling back to use VA rotated buffer
    bool fallback = false;
    float scaleX = (float)mSrcCrop.w / mPosition.w;
    float scaleY = (float)mSrcCrop.h / mPosition.h;
    if (mTransform == HAL_TRANSFORM_ROT_270 || mTransform == HAL_TRANSFORM_ROT_90) {
        scaleX = (float)mSrcCrop.w / mPosition.h;
        scaleY = (float)mSrcCrop.h / mPosition.w;
    }
    if (scaleX >= 3 || scaleY >= 3 || scaleX < 1.0/3 || scaleY < 1.0/3) {
        if (mUseOverlayRotation) {
            DLOGTRACE("overlay rotation with scaling >= 3, use VA rotated buffer");
        }
        fallback = true;
    } else if ((int)mSrcCrop.x & 63) {
        if (mUseOverlayRotation) {
            DLOGTRACE("offset is not 64 bytes aligned, use VA rotated buffer");
        }
        fallback = true;
    }
#if 0
    else if (mTransform != HAL_TRANSFORM_ROT_180 && scaleX != scaleY) {
        if (mUseOverlayRotation) {
            DLOGTRACE("overlay rotation with uneven scaling, use VA rotated buffer");
        }
        fallback = true;
    }
#endif

    // per DC spec, if video is 1080(H)x1920(V), the buffer
    // need 1920 of 64-pixel strip if using hw rotation.
    // fallback to video ration buffer in such case.
    if (mSrcCrop.w == 1080 && mSrcCrop.h == 1920 && mTransform != 0) {
        DLOGTRACE("1080(H)x1920(V) cannot use hw rotation, use VA rotated buffer");
        fallback = true;
    }

    if (fallback || mBobDeinterlace) {
        mUseOverlayRotation = false;
        mRotationConfig = 0;
    } else {
        mUseOverlayRotation = true;
    }
    return mUseOverlayRotation;
}

bool AnnOverlayPlane::flush(uint32_t flags)
{
    RETURN_FALSE_IF_NOT_INIT();
    ALOGTRACE("flags = %#x, type = %d, index = %d", flags, mType, mIndex);

    if (!(flags & PLANE_ENABLE) && !(flags & PLANE_DISABLE)) {
        ELOGTRACE("invalid flush flags.");
        return false;
    }

    struct drm_psb_register_rw_arg arg;
    memset(&arg, 0, sizeof(struct drm_psb_register_rw_arg));

    if (flags & PLANE_DISABLE)
        arg.plane_disable_mask = 1;
    else if (flags & PLANE_ENABLE)
        arg.plane_enable_mask = 1;

    arg.plane.type = DC_OVERLAY_PLANE;
    arg.plane.index = mIndex;
    arg.plane.ctx = mContext.ctx.ov_ctx.ovadd;
    if (flags & PLANE_DISABLE) {
        DLOGTRACE("disabling overlay %d on device %d", mIndex, mDevice);
    }

    // issue ioctl
    Drm *drm = Hwcomposer::getInstance().getDrm();
    bool ret = drm->writeReadIoctl(DRM_PSB_REGISTER_RW, &arg, sizeof(arg));
    if (ret == false) {
        WLOGTRACE("overlay update failed with error code %d", ret);
        return false;
    }

    return true;
}

} // namespace intel
} // namespace android
