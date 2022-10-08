/*
* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above
*      copyright notice, this list of conditions and the following
*      disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <math.h>
#include <utils/Log.h>
#include <linux/msm_mdp.h>
#include <cutils/properties.h>
#include "gralloc_priv.h"
#include "overlayUtils.h"
#include "mdpWrapper.h"
#include "mdp_version.h"

// just a helper static thingy
namespace {
struct IOFile {
    IOFile(const char* s, const char* mode) : fp(0) {
        fp = ::fopen(s, mode);
        if(!fp) {
            ALOGE("Failed open %s", s);
        }
    }
    template <class T>
            size_t read(T& r, size_t elem) {
                if(fp) {
                    return ::fread(&r, sizeof(T), elem, fp);
                }
                return 0;
            }
    size_t write(const char* s, uint32_t val) {
        if(fp) {
            return ::fprintf(fp, s, val);
        }
        return 0;
    }
    bool valid() const { return fp != 0; }
    ~IOFile() {
        if(fp) ::fclose(fp);
        fp=0;
    }
    FILE* fp;
};
}

namespace overlay {

//----------From class Res ------------------------------
const char* const Res::fbPath = "/dev/graphics/fb%u";
const char* const Res::rotPath = "/dev/msm_rotator";
const char* const Res::format3DFile =
        "/sys/class/graphics/fb1/format_3d";
const char* const Res::edid3dInfoFile =
        "/sys/class/graphics/fb1/3d_present";
const char* const Res::barrierFile =
        "/sys/devices/platform/mipi_novatek.0/enable_3d_barrier";
//--------------------------------------------------------



namespace utils {

//--------------------------------------------------------
//Refer to graphics.h, gralloc_priv.h, msm_mdp.h
int getMdpFormat(int format) {
    switch (format) {
        //From graphics.h
        case HAL_PIXEL_FORMAT_RGBA_8888 :
            return MDP_RGBA_8888;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return MDP_RGBX_8888;
        case HAL_PIXEL_FORMAT_RGB_888:
            return MDP_RGB_888;
        case HAL_PIXEL_FORMAT_RGB_565:
            return MDP_RGB_565;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            return MDP_BGRA_8888;
        case HAL_PIXEL_FORMAT_YV12:
            return MDP_Y_CR_CB_GH2V2;
        case HAL_PIXEL_FORMAT_YCbCr_422_SP:
            return MDP_Y_CBCR_H2V1;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            return MDP_Y_CRCB_H2V2;

        //From gralloc_priv.h
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
            return MDP_Y_CBCR_H2V2_TILE;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            return MDP_Y_CBCR_H2V2;
        case HAL_PIXEL_FORMAT_YCrCb_422_SP:
            return MDP_Y_CRCB_H2V1;
        case HAL_PIXEL_FORMAT_YCbCr_444_SP:
            return MDP_Y_CBCR_H1V1;
        case HAL_PIXEL_FORMAT_YCrCb_444_SP:
            return MDP_Y_CRCB_H1V1;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS:
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:
            //NV12 encodeable format maps to the venus format on
            //B-Family targets
            return MDP_Y_CBCR_H2V2_VENUS;
        default:
            //Unsupported by MDP
            //---graphics.h--------
            //HAL_PIXEL_FORMAT_RGBA_5551
            //HAL_PIXEL_FORMAT_RGBA_4444
            //HAL_PIXEL_FORMAT_YCbCr_422_I
            //---gralloc_priv.h-----
            //HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO    = 0x7FA30C01
            //HAL_PIXEL_FORMAT_R_8                    = 0x10D
            //HAL_PIXEL_FORMAT_RG_88                  = 0x10E
            ALOGE("%s: Unsupported HAL format = 0x%x", __func__, format);
            return -1;
    }
    // not reached
    return -1;
}

//Takes mdp format as input and translates to equivalent HAL format
//Refer to graphics.h, gralloc_priv.h, msm_mdp.h for formats.
int getHALFormat(int mdpFormat) {
    switch (mdpFormat) {
        //From graphics.h
        case MDP_RGBA_8888:
            return HAL_PIXEL_FORMAT_RGBA_8888;
        case MDP_RGBX_8888:
            return HAL_PIXEL_FORMAT_RGBX_8888;
        case MDP_RGB_888:
            return HAL_PIXEL_FORMAT_RGB_888;
        case MDP_RGB_565:
            return HAL_PIXEL_FORMAT_RGB_565;
        case MDP_BGRA_8888:
            return HAL_PIXEL_FORMAT_BGRA_8888;
        case MDP_Y_CR_CB_GH2V2:
            return HAL_PIXEL_FORMAT_YV12;
        case MDP_Y_CBCR_H2V1:
            return HAL_PIXEL_FORMAT_YCbCr_422_SP;
        case MDP_Y_CRCB_H2V2:
            return HAL_PIXEL_FORMAT_YCrCb_420_SP;

        //From gralloc_priv.h
        case MDP_Y_CBCR_H2V2_TILE:
            return HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED;
        case MDP_Y_CBCR_H2V2:
            return HAL_PIXEL_FORMAT_YCbCr_420_SP;
        case MDP_Y_CRCB_H2V1:
            return HAL_PIXEL_FORMAT_YCrCb_422_SP;
        case MDP_Y_CBCR_H1V1:
            return HAL_PIXEL_FORMAT_YCbCr_444_SP;
        case MDP_Y_CRCB_H1V1:
            return HAL_PIXEL_FORMAT_YCrCb_444_SP;
        case MDP_Y_CBCR_H2V2_VENUS:
            return HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS;
        default:
            ALOGE("%s: Unsupported MDP format = 0x%x", __func__, mdpFormat);
            return -1;
    }
    // not reached
    return -1;
}

int getMdpOrient(eTransform rotation) {
    int retTrans = 0;
    bool trans90 = false;
    int mdpVersion = qdutils::MDPVersion::getInstance().getMDPVersion();
    bool aFamily = (mdpVersion < qdutils::MDSS_V5);

    ALOGD_IF(DEBUG_OVERLAY, "%s: In rotation = %d", __FUNCTION__, rotation);
    if(rotation & OVERLAY_TRANSFORM_ROT_90) {
        retTrans |= MDP_ROT_90;
        trans90 = true;
    }

    if(rotation & OVERLAY_TRANSFORM_FLIP_H) {
        if(trans90 && aFamily) {
            //Swap for a-family, since its driver does 90 first
            retTrans |= MDP_FLIP_UD;
        } else {
            retTrans |= MDP_FLIP_LR;
        }
    }

    if(rotation & OVERLAY_TRANSFORM_FLIP_V) {
        if(trans90 && aFamily) {
            //Swap for a-family, since its driver does 90 first
            retTrans |= MDP_FLIP_LR;
        } else {
            retTrans |= MDP_FLIP_UD;
        }
    }

    ALOGD_IF(DEBUG_OVERLAY, "%s: Out rotation = %d", __FUNCTION__, retTrans);
    return retTrans;
}

int getDownscaleFactor(const int& src_w, const int& src_h,
        const int& dst_w, const int& dst_h) {
    int dscale_factor = utils::ROT_DS_NONE;
    // The tolerance is an empirical grey area that needs to be adjusted
    // manually so that we always err on the side of caution
    float fDscaleTolerance = 0.05;
    // We need this check to engage the rotator whenever possible to assist MDP
    // in performing video downscale.
    // This saves bandwidth and avoids causing the driver to make too many panel
    // -mode switches between BLT (writeback) and non-BLT (Direct) modes.
    // Use-case: Video playback [with downscaling and rotation].
    if (dst_w && dst_h)
    {
        float fDscale =  sqrtf((float)(src_w * src_h) / (float)(dst_w * dst_h)) +
                         fDscaleTolerance;

        // On our MTP 1080p playback case downscale after sqrt is coming to 1.87
        // we were rounding to 1. So entirely MDP has to do the downscaling.
        // BW requirement and clock requirement is high across MDP4 targets.
        // It is unable to downscale 1080p video to panel resolution on 8960.
        // round(x) will round it to nearest integer and avoids above issue.
        uint32_t dscale = round(fDscale);

        if(dscale < 2) {
            // Down-scale to > 50% of orig.
            dscale_factor = utils::ROT_DS_NONE;
        } else if(dscale < 4) {
            // Down-scale to between > 25% to <= 50% of orig.
            dscale_factor = utils::ROT_DS_HALF;
        } else if(dscale < 8) {
            // Down-scale to between > 12.5% to <= 25% of orig.
            dscale_factor = utils::ROT_DS_FOURTH;
        } else {
            // Down-scale to <= 12.5% of orig.
            dscale_factor = utils::ROT_DS_EIGHTH;
        }
    }
    return dscale_factor;
}

void getDecimationFactor(const int& src_w, const int& src_h,
        const int& dst_w, const int& dst_h, uint8_t& horzDeci,
        uint8_t& vertDeci) {
    horzDeci = 0;
    vertDeci = 0;
    float horDscale = ceilf((float)src_w / (float)dst_w);
    float verDscale = ceilf((float)src_h / (float)dst_h);

    //Next power of 2, if not already
    horDscale = powf(2.0f, ceilf(log2f(horDscale)));
    verDscale = powf(2.0f, ceilf(log2f(verDscale)));

    //Since MDP can do 1/4 dscale and has better quality, split the task
    //between decimator and MDP downscale
    horDscale /= 4.0f;
    verDscale /= 4.0f;

    if((int)horDscale)
        horzDeci = (uint8_t)log2f(horDscale);

    if((int)verDscale)
        vertDeci = (uint8_t)log2f(verDscale);

    if(src_w > 2048) {
        //If the client sends us something > what a layer mixer supports
        //then it means it doesn't want to use split-pipe but wants us to
        //decimate. A minimum decimation of 2 will ensure that the width is
        //always within layer mixer limits.
        if(horzDeci < 2)
            horzDeci = 2;
    }
}

static inline int compute(const uint32_t& x, const uint32_t& y,
        const uint32_t& z) {
    return x - ( y + z );
}

void preRotateSource(const eTransform& tr, Whf& whf, Dim& srcCrop) {
    if(tr & OVERLAY_TRANSFORM_FLIP_H) {
        srcCrop.x = compute(whf.w, srcCrop.x, srcCrop.w);
    }
    if(tr & OVERLAY_TRANSFORM_FLIP_V) {
        srcCrop.y = compute(whf.h, srcCrop.y, srcCrop.h);
    }
    if(tr & OVERLAY_TRANSFORM_ROT_90) {
        int tmp = srcCrop.x;
        srcCrop.x = compute(whf.h,
                srcCrop.y,
                srcCrop.h);
        srcCrop.y = tmp;
        swap(whf.w, whf.h);
        swap(srcCrop.w, srcCrop.h);
    }
}

bool is3DTV() {
    char is3DTV = '0';
    IOFile fp(Res::edid3dInfoFile, "r");
    (void)fp.read(is3DTV, 1);
    ALOGI("3DTV EDID flag: %d", is3DTV);
    return (is3DTV == '0') ? false : true;
}

bool isPanel3D() {
    OvFD fd;
    if(!overlay::open(fd, 0 /*fb*/, Res::fbPath)){
        ALOGE("isPanel3D Can't open framebuffer 0");
        return false;
    }
    fb_fix_screeninfo finfo;
    if(!mdp_wrapper::getFScreenInfo(fd.getFD(), finfo)) {
        ALOGE("isPanel3D read fb0 failed");
    }
    fd.close();
    return (FB_TYPE_3D_PANEL == finfo.type) ? true : false;
}

bool usePanel3D() {
    if(!isPanel3D())
        return false;
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.user.panel3D", value, "0");
    int usePanel3D = atoi(value);
    return usePanel3D ? true : false;
}

bool send3DInfoPacket (uint32_t format3D) {
    IOFile fp(Res::format3DFile, "wb");
    (void)fp.write("%d", format3D);
    if(!fp.valid()) {
        ALOGE("send3DInfoPacket: no sysfs entry for setting 3d mode");
        return false;
    }
    return true;
}

bool enableBarrier (uint32_t orientation) {
    IOFile fp(Res::barrierFile, "wb");
    (void)fp.write("%d", orientation);
    if(!fp.valid()) {
        ALOGE("enableBarrier no sysfs entry for "
                "enabling barriers on 3D panel");
        return false;
    }
    return true;
}

uint32_t getS3DFormat(uint32_t fmt) {
    // The S3D is part of the HAL_PIXEL_FORMAT_YV12 value. Add
    // an explicit check for the format
    if (fmt == HAL_PIXEL_FORMAT_YV12) {
        return 0;
    }
    uint32_t fmt3D = format3D(fmt);
    uint32_t fIn3D = format3DInput(fmt3D); // MSB 2 bytes - inp
    uint32_t fOut3D = format3DOutput(fmt3D); // LSB 2 bytes - out
    fmt3D = fIn3D | fOut3D;
    if (!fIn3D) {
        fmt3D |= fOut3D << SHIFT_TOT_3D; //Set the input format
    }
    if (!fOut3D) {
        switch (fIn3D) {
            case HAL_3D_IN_SIDE_BY_SIDE_L_R:
            case HAL_3D_IN_SIDE_BY_SIDE_R_L:
                // For all side by side formats, set the output
                // format as Side-by-Side i.e 0x1
                fmt3D |= HAL_3D_IN_SIDE_BY_SIDE_L_R >> SHIFT_TOT_3D;
                break;
            default:
                fmt3D |= fIn3D >> SHIFT_TOT_3D; //Set the output format
        }
    }
    return fmt3D;
}

void getDump(char *buf, size_t len, const char *prefix,
        const mdp_overlay& ov) {
    char str[256] = {'\0'};
    snprintf(str, 256,
            "%s id=%d z=%d fg=%d alpha=%d mask=%d flags=0x%x H.Deci=%d,"
            "V.Deci=%d\n",
            prefix, ov.id, ov.z_order, ov.is_fg, ov.alpha,
            ov.transp_mask, ov.flags, ov.horz_deci, ov.vert_deci);
    strncat(buf, str, strlen(str));
    getDump(buf, len, "\tsrc", ov.src);
    getDump(buf, len, "\tsrc_rect", ov.src_rect);
    getDump(buf, len, "\tdst_rect", ov.dst_rect);
}

void getDump(char *buf, size_t len, const char *prefix,
        const msmfb_img& ov) {
    char str_src[256] = {'\0'};
    snprintf(str_src, 256,
            "%s w=%d h=%d format=%d %s\n",
            prefix, ov.width, ov.height, ov.format,
            overlay::utils::getFormatString(ov.format));
    strncat(buf, str_src, strlen(str_src));
}

void getDump(char *buf, size_t len, const char *prefix,
        const mdp_rect& ov) {
    char str_rect[256] = {'\0'};
    snprintf(str_rect, 256,
            "%s x=%d y=%d w=%d h=%d\n",
            prefix, ov.x, ov.y, ov.w, ov.h);
    strncat(buf, str_rect, strlen(str_rect));
}

void getDump(char *buf, size_t len, const char *prefix,
        const msmfb_overlay_data& ov) {
    char str[256] = {'\0'};
    snprintf(str, 256,
            "%s id=%d\n",
            prefix, ov.id);
    strncat(buf, str, strlen(str));
    getDump(buf, len, "\tdata", ov.data);
}

void getDump(char *buf, size_t len, const char *prefix,
        const msmfb_data& ov) {
    char str_data[256] = {'\0'};
    snprintf(str_data, 256,
            "%s offset=%d memid=%d id=%d flags=0x%x\n",
            prefix, ov.offset, ov.memory_id, ov.id, ov.flags);
    strncat(buf, str_data, strlen(str_data));
}

void getDump(char *buf, size_t len, const char *prefix,
        const msm_rotator_img_info& rot) {
    char str[256] = {'\0'};
    snprintf(str, 256, "%s sessid=%u rot=%d, enable=%d downscale=%d\n",
            prefix, rot.session_id, rot.rotations, rot.enable,
            rot.downscale_ratio);
    strncat(buf, str, strlen(str));
    getDump(buf, len, "\tsrc", rot.src);
    getDump(buf, len, "\tdst", rot.dst);
    getDump(buf, len, "\tsrc_rect", rot.src_rect);
}

void getDump(char *buf, size_t len, const char *prefix,
        const msm_rotator_data_info& rot) {
    char str[256] = {'\0'};
    snprintf(str, 256,
            "%s sessid=%u\n",
            prefix, rot.session_id);
    strncat(buf, str, strlen(str));
    getDump(buf, len, "\tsrc", rot.src);
    getDump(buf, len, "\tdst", rot.dst);
}

//Helper to even out x,w and y,h pairs
//x,y are always evened to ceil and w,h are evened to floor
void normalizeCrop(uint32_t& xy, uint32_t& wh) {
    if(xy & 1) {
        even_ceil(xy);
        if(wh & 1)
            even_floor(wh);
        else
            wh -= 2;
    } else {
        even_floor(wh);
    }
}

} // utils

} // overlay
